#!/usr/bin/env python3
"""Asynchronous black-box load tester for ft_irc.

Examples:
    python3 tests/loadtest.py 127.0.0.1 6670 pass --clients 500
    python3 tests/loadtest.py 127.0.0.1 6670 pass --clients 1000 \
        --mode fanout --channel-size 100 --rate 500 --duration 60
    python3 tests/loadtest.py 127.0.0.1 6670 pass --clients 500 \
        --mode direct --rate 5000 --slow-readers 10 --json result.json

The reported message rate is the rate of PRIVMSG commands entering the server.
For fan-out tests, ``expected_deliveries`` shows the larger output workload.
"""

import argparse
import asyncio
import json
import math
import random
import statistics
import sys
import time


IRC_LINE_LIMIT = 512
SAMPLE_LIMIT = 100000


class Stats(object):
    def __init__(self):
        self.connect_attempts = 0
        self.connected = 0
        self.connect_failed = 0
        self.registered = 0
        self.joined = 0
        self.sent = 0
        self.send_failed = 0
        self.offered_deliveries = 0
        self.expected_deliveries = 0
        self.received_deliveries = 0
        self.duplicates_or_reordered = 0
        self.sequence_gaps = 0
        self.unexpected_disconnects = 0
        self.probes_sent = 0
        self.probes_received = 0
        self.latencies_ms = []
        self.probe_latencies_ms = []
        self.errors = []
        self._latency_seen = 0
        self._probe_seen = 0

    def error(self, text):
        if len(self.errors) < 20:
            self.errors.append(str(text))

    def sample_latency(self, value, probe=False):
        samples = self.probe_latencies_ms if probe else self.latencies_ms
        counter_name = "_probe_seen" if probe else "_latency_seen"
        seen = getattr(self, counter_name) + 1
        setattr(self, counter_name, seen)
        if len(samples) < SAMPLE_LIMIT:
            samples.append(value)
        else:
            slot = random.randint(1, seen)
            if slot <= SAMPLE_LIMIT:
                samples[slot - 1] = value


class LoadClient(object):
    def __init__(self, index, nick, reader, writer, stats):
        self.index = index
        self.nick = nick
        self.reader = reader
        self.writer = writer
        self.stats = stats
        self.channel = None
        self.registered = asyncio.Event()
        self.joined = asyncio.Event()
        self.reader_task = None
        self.slow = False
        self.closing = False
        self.last_sequence = {}

    def send_line(self, line):
        data = (line + "\r\n").encode("utf-8")
        if len(data) > IRC_LINE_LIMIT:
            raise ValueError("outgoing IRC line exceeds 512 bytes")
        self.writer.write(data)

    async def read_loop(self):
        try:
            while True:
                raw = await self.reader.readline()
                if not raw:
                    if not self.closing and not self.slow:
                        self.stats.unexpected_disconnects += 1
                    return
                if len(raw) > IRC_LINE_LIMIT:
                    self.stats.error("server sent a line longer than 512 bytes")
                self.handle_line(raw.decode("utf-8", errors="replace").rstrip("\r\n"))
        except asyncio.CancelledError:
            return
        except Exception as exc:
            if not self.closing and not self.slow:
                self.stats.unexpected_disconnects += 1
                self.stats.error("reader {0}: {1}".format(self.nick, exc))

    def handle_line(self, line):
        parts = line.split()
        if not parts:
            return
        command_index = 1 if parts[0].startswith(":") else 0
        if command_index >= len(parts):
            return
        command = parts[command_index].upper()

        if command == "001":
            self.registered.set()
            return
        if command == "JOIN":
            prefix_nick = parts[0][1:].split("!", 1)[0] if parts[0].startswith(":") else ""
            if prefix_nick == self.nick:
                self.joined.set()
            return
        if command == "PONG":
            marker = line.rsplit(":", 1)[-1]
            if marker.startswith("LT-PING|"):
                fields = marker.split("|")
                if len(fields) == 3:
                    try:
                        sent_ns = int(fields[2])
                    except ValueError:
                        return
                    self.stats.probes_received += 1
                    self.stats.sample_latency(
                        (time.monotonic_ns() - sent_ns) / 1000000.0, probe=True
                    )
            return
        if command != "PRIVMSG" or " :" not in line:
            return

        payload = line.split(" :", 1)[1]
        if not payload.startswith("LT|"):
            return
        fields = payload.split("|", 4)
        if len(fields) < 4:
            return
        stream = fields[1]
        try:
            sequence = int(fields[2])
            sent_ns = int(fields[3])
        except ValueError:
            return

        previous = self.last_sequence.get(stream)
        if previous is not None:
            if sequence <= previous:
                self.stats.duplicates_or_reordered += 1
            elif sequence > previous + 1:
                self.stats.sequence_gaps += sequence - previous - 1
        if previous is None or sequence > previous:
            self.last_sequence[stream] = sequence
        self.stats.received_deliveries += 1
        self.stats.sample_latency((time.monotonic_ns() - sent_ns) / 1000000.0)

    def stop_reading(self):
        self.slow = True
        if self.reader_task is not None:
            self.reader_task.cancel()

    async def close(self):
        self.closing = True
        if self.reader_task is not None:
            self.reader_task.cancel()
        self.writer.close()
        try:
            await self.writer.wait_closed()
        except Exception:
            pass


def percentile(values, fraction):
    if not values:
        return None
    ordered = sorted(values)
    index = int(math.ceil(fraction * len(ordered))) - 1
    return ordered[max(0, min(index, len(ordered) - 1))]


def latency_summary(values):
    if not values:
        return {"samples": 0, "median": None, "p95": None, "p99": None, "max": None}
    return {
        "samples": len(values),
        "median": round(statistics.median(values), 3),
        "p95": round(percentile(values, 0.95), 3),
        "p99": round(percentile(values, 0.99), 3),
        "max": round(max(values), 3),
    }


async def wait_for_event(client, event, timeout, phase):
    try:
        await asyncio.wait_for(event.wait(), timeout)
        return True
    except asyncio.TimeoutError:
        client.stats.error("{0} timed out for {1}".format(client.nick, phase))
        return False


async def connect_one(args, index, stats):
    stats.connect_attempts += 1
    nick = "lt{0:07d}".format(index)
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(args.host, args.port, limit=IRC_LINE_LIMIT * 4),
            args.timeout,
        )
    except Exception as exc:
        stats.connect_failed += 1
        stats.error("connect {0}: {1}".format(nick, exc))
        return None

    stats.connected += 1
    client = LoadClient(index, nick, reader, writer, stats)
    client.reader_task = asyncio.create_task(client.read_loop())
    client.send_line("PASS " + args.password)
    client.send_line("NICK " + nick)
    client.send_line("USER {0} 0 * :Load test {1}".format(nick, index))
    try:
        await writer.drain()
    except Exception as exc:
        stats.error("registration write {0}: {1}".format(nick, exc))
    return client


async def connect_clients(args, stats):
    clients = []
    interval = 1.0 / args.connect_rate
    next_start = time.monotonic()
    pending = set()

    for index in range(args.clients):
        now = time.monotonic()
        if now < next_start:
            await asyncio.sleep(next_start - now)
        task = asyncio.create_task(connect_one(args, index, stats))
        pending.add(task)
        next_start += interval
        if len(pending) >= args.connect_concurrency:
            done, _ = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)
            clients.extend(result for result in (task.result() for task in done) if result)
            pending.difference_update(done)

    if pending:
        done, _ = await asyncio.wait(pending)
        clients.extend(result for result in (task.result() for task in done) if result)
    clients.sort(key=lambda client: client.index)

    results = await asyncio.gather(
        *(wait_for_event(client, client.registered, args.timeout, "registration") for client in clients)
    )
    registered = [client for client, ok in zip(clients, results) if ok]
    stats.registered = len(registered)
    return clients, registered


async def join_channels(args, clients, stats):
    for position, client in enumerate(clients):
        group = position // args.channel_size
        client.channel = "#load{0}".format(group)
        client.send_line("JOIN " + client.channel)
    await asyncio.gather(*(client.writer.drain() for client in clients), return_exceptions=True)
    results = await asyncio.gather(
        *(wait_for_event(client, client.joined, args.timeout, "JOIN") for client in clients)
    )
    joined = [client for client, ok in zip(clients, results) if ok]
    stats.joined = len(joined)
    return joined


def make_streams(args, clients):
    streams = []
    if args.mode == "direct":
        for index in range(0, len(clients) - 1, 2):
            sender = clients[index]
            target = clients[index + 1]
            if not sender.slow:
                streams.append(("d{0}".format(index // 2), sender, target.nick, [target]))
        return streams

    groups = {}
    for client in clients:
        groups.setdefault(client.channel, []).append(client)
    for index, channel in enumerate(sorted(groups)):
        members = groups[channel]
        readable_senders = [client for client in members if not client.slow]
        if len(members) >= 2 and readable_senders:
            sender = readable_senders[0]
            recipients = [client for client in members if client is not sender]
            streams.append(("c{0}".format(index), sender, channel, recipients))
    return streams


async def probe_loop(client, stats, stop_event, interval):
    serial = 0
    while not stop_event.is_set():
        sent_ns = time.monotonic_ns()
        try:
            client.send_line("PING :LT-PING|{0}|{1}".format(serial, sent_ns))
            stats.probes_sent += 1
        except Exception as exc:
            stats.error("probe send: {0}".format(exc))
            return
        serial += 1
        try:
            await asyncio.wait_for(stop_event.wait(), interval)
        except asyncio.TimeoutError:
            pass


async def run_traffic(args, streams, stats, probe):
    if not streams:
        raise RuntimeError("not enough registered clients to form a traffic stream")
    stop_probe = asyncio.Event()
    probe_task = asyncio.create_task(probe_loop(probe, stats, stop_probe, args.probe_interval))
    sequences = [0] * len(streams)
    stream_index = 0
    padding = "x" * args.payload_size
    started = time.monotonic()
    last = started
    deadline = started + args.duration
    next_flush = started + 0.25
    credit = 0.0
    senders = list({stream[1] for stream in streams})

    while time.monotonic() < deadline:
        now = time.monotonic()
        credit = min(credit + (now - last) * args.rate, max(1.0, args.rate * 0.25))
        last = now
        batch = int(credit)
        if batch == 0:
            await asyncio.sleep(min(0.01, max(0.0, deadline - now)))
            continue
        credit -= batch

        for _ in range(batch):
            slot = stream_index % len(streams)
            stream, sender, target, recipients = streams[slot]
            sequence = sequences[slot]
            payload = "LT|{0}|{1}|{2}|{3}".format(
                stream, sequence, time.monotonic_ns(), padding
            )
            try:
                sender.send_line("PRIVMSG {0} :{1}".format(target, payload))
                stats.sent += 1
                stats.offered_deliveries += len(recipients)
                stats.expected_deliveries += sum(1 for client in recipients if not client.slow)
            except Exception as exc:
                stats.send_failed += 1
                stats.error("traffic send {0}: {1}".format(sender.nick, exc))
            sequences[slot] += 1
            stream_index += 1
        if now >= next_flush:
            await asyncio.gather(
                *(sender.writer.drain() for sender in senders), return_exceptions=True
            )
            next_flush = time.monotonic() + 0.25
        await asyncio.sleep(0)

    elapsed = time.monotonic() - started
    stop_probe.set()
    await probe_task
    await asyncio.gather(
        *(sender.writer.drain() for sender in senders), return_exceptions=True
    )
    await asyncio.sleep(args.settle)
    return elapsed


def build_summary(args, stats, elapsed, wall_elapsed):
    expected = stats.expected_deliveries
    received = stats.received_deliveries
    return {
        "configuration": {
            "host": args.host,
            "port": args.port,
            "clients": args.clients,
            "mode": args.mode,
            "channel_size": args.channel_size if args.mode == "fanout" else None,
            "slow_readers": args.slow_readers,
            "target_input_messages_per_second": args.rate,
            "duration_seconds": args.duration,
            "payload_size": args.payload_size,
        },
        "connections": {
            "attempted": stats.connect_attempts,
            "connected": stats.connected,
            "failed": stats.connect_failed,
            "registered": stats.registered,
            "joined": stats.joined if args.mode == "fanout" else None,
            "unexpected_disconnects": stats.unexpected_disconnects,
        },
        "traffic": {
            "sent_commands": stats.sent,
            "send_failures": stats.send_failed,
            "actual_input_messages_per_second": round(stats.sent / elapsed, 2) if elapsed else 0,
            "offered_deliveries_including_slow_readers": stats.offered_deliveries,
            "expected_deliveries_to_readers": expected,
            "received_deliveries": received,
            "delivery_ratio": round(received / expected, 6) if expected else None,
            "duplicates_or_reordered": stats.duplicates_or_reordered,
            "internal_sequence_gaps": stats.sequence_gaps,
        },
        "delivery_latency_ms": latency_summary(stats.latencies_ms),
        "probe": {
            "sent": stats.probes_sent,
            "received": stats.probes_received,
            "latency_ms": latency_summary(stats.probe_latencies_ms),
        },
        "wall_time_seconds": round(wall_elapsed, 3),
        "errors": stats.errors,
    }


def print_summary(summary):
    connections = summary["connections"]
    traffic = summary["traffic"]
    latency = summary["delivery_latency_ms"]
    probe = summary["probe"]
    print("\n=== ft_irc load-test summary ===")
    print(
        "Connections: {0}/{1} connected, {2} registered, {3} failed, {4} disconnected".format(
            connections["connected"],
            connections["attempted"],
            connections["registered"],
            connections["failed"],
            connections["unexpected_disconnects"],
        )
    )
    if connections["joined"] is not None:
        print("Channel joins: {0}".format(connections["joined"]))
    print(
        "Input: {0} commands at {1} msg/s ({2} send failures)".format(
            traffic["sent_commands"],
            traffic["actual_input_messages_per_second"],
            traffic["send_failures"],
        )
    )
    print(
        "Delivery: {0}/{1} readable ({2}); {3} offered including slow readers".format(
            traffic["received_deliveries"],
            traffic["expected_deliveries_to_readers"],
            traffic["delivery_ratio"] if traffic["delivery_ratio"] is not None else "n/a",
            traffic["offered_deliveries_including_slow_readers"],
        )
    )
    print(
        "Integrity: gaps {0}, duplicate/reordered {1}".format(
            traffic["internal_sequence_gaps"],
            traffic["duplicates_or_reordered"],
        )
    )
    print(
        "Delivery latency ms: median {0}, p95 {1}, p99 {2}, max {3} ({4} samples)".format(
            latency["median"], latency["p95"], latency["p99"], latency["max"], latency["samples"]
        )
    )
    print(
        "PING probes: {0}/{1}; p99 {2} ms, max {3} ms".format(
            probe["received"],
            probe["sent"],
            probe["latency_ms"]["p99"],
            probe["latency_ms"]["max"],
        )
    )
    if summary["errors"]:
        print("Errors (first {0}):".format(len(summary["errors"])))
        for error in summary["errors"]:
            print("  - " + error)


async def async_main(args):
    stats = Stats()
    all_clients = []
    started = time.monotonic()
    elapsed = 0.0
    try:
        all_clients, registered = await connect_clients(args, stats)
        active = registered
        if args.mode == "fanout":
            active = await join_channels(args, registered, stats)
        if len(active) < 2:
            raise RuntimeError("fewer than two clients completed setup")

        slow_count = min(args.slow_readers, max(0, len(active) - 2))
        if args.mode == "direct":
            slow_candidates = active[1::2]
            slow_count = min(slow_count, len(slow_candidates))
            slow_clients = slow_candidates[-slow_count:] if slow_count else []
        else:
            slow_clients = active[-slow_count:] if slow_count else []
        for client in slow_clients:
            client.stop_reading()
        await asyncio.sleep(0)

        streams = make_streams(args, active)
        probe_candidates = [client for client in active if not client.slow]
        elapsed = await run_traffic(args, streams, stats, probe_candidates[-1])
    except Exception as exc:
        stats.error(exc)
    finally:
        await asyncio.gather(*(client.close() for client in all_clients), return_exceptions=True)

    summary = build_summary(args, stats, elapsed, time.monotonic() - started)
    print_summary(summary)
    if args.json_path:
        with open(args.json_path, "w", encoding="utf-8") as output:
            json.dump(summary, output, indent=2, sort_keys=True)
            output.write("\n")

    if not args.fail_on_errors:
        return 0
    traffic = summary["traffic"]
    delivery_failed = (
        traffic["expected_deliveries_to_readers"] != traffic["received_deliveries"]
        or traffic["duplicates_or_reordered"] != 0
        or traffic["internal_sequence_gaps"] != 0
    )
    setup_failed = stats.registered != args.clients or (
        args.mode == "fanout" and stats.joined != stats.registered
    )
    return 1 if setup_failed or delivery_failed or stats.errors else 0


def positive_int(value):
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return parsed


def nonnegative_int(value):
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("must be zero or greater")
    return parsed


def positive_float(value):
    parsed = float(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return parsed


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", nargs="?", default="127.0.0.1")
    parser.add_argument("port", nargs="?", type=int, default=6667)
    parser.add_argument("password", nargs="?", default="testpass")
    parser.add_argument("--clients", type=positive_int, default=100)
    parser.add_argument("--connect-rate", type=positive_float, default=100.0,
                        help="new connection attempts per second")
    parser.add_argument("--connect-concurrency", type=positive_int, default=100)
    parser.add_argument("--mode", choices=("fanout", "direct"), default="fanout")
    parser.add_argument("--channel-size", type=positive_int, default=50)
    parser.add_argument("--rate", type=positive_float, default=100.0,
                        help="total PRIVMSG commands per second")
    parser.add_argument("--duration", type=positive_float, default=30.0)
    parser.add_argument("--payload-size", type=nonnegative_int, default=64,
                        help="padding bytes in each generated message")
    parser.add_argument("--slow-readers", type=nonnegative_int, default=0)
    parser.add_argument("--probe-interval", type=positive_float, default=1.0)
    parser.add_argument("--timeout", type=positive_float, default=5.0,
                        help="connect, registration, and JOIN timeout")
    parser.add_argument("--settle", type=positive_float, default=2.0,
                        help="seconds to wait for final deliveries")
    parser.add_argument("--json", dest="json_path", metavar="PATH",
                        help="also write the summary as JSON")
    parser.add_argument("--fail-on-errors", action="store_true",
                        help="exit nonzero for setup errors or imperfect delivery")
    args = parser.parse_args()
    if args.port < 1 or args.port > 65535:
        parser.error("port must be between 1 and 65535")
    if args.channel_size < 2 and args.mode == "fanout":
        parser.error("--channel-size must be at least 2 in fanout mode")
    if args.slow_readers >= args.clients - 1:
        parser.error("leave at least two normal readers")
    # Prefix, command, target, marker fields, and CRLF need about 100 bytes.
    if args.payload_size > 400:
        parser.error("--payload-size must be 400 or less to stay within IRC's 512-byte limit")
    return args


def main():
    args = parse_args()
    try:
        return asyncio.run(async_main(args))
    except KeyboardInterrupt:
        print("\nInterrupted", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
