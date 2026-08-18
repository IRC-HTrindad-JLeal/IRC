# IRC Load Tester

`loadtest.py` is a dependency-free asynchronous stress tester. It creates many
registered IRC users, sends messages at a controlled rate, verifies deliveries,
and measures latency without requiring hundreds of shell processes.

## Quick start

Start the server with logging redirected so terminal output does not become the
bottleneck:

```bash
./ircserv 6670 pass >server.log 2>&1
```

Then run a small fan-out test:

```bash
python3 tests/loadtest.py 127.0.0.1 6670 pass \
  --clients 100 \
  --channel-size 25 \
  --rate 500 \
  --duration 30
```

## Workload modes

`fanout` is the default. Clients are divided into channels and one sender in each
channel produces messages for every other member:

```bash
python3 tests/loadtest.py 127.0.0.1 6670 pass \
  --clients 500 --mode fanout --channel-size 50 \
  --rate 1000 --duration 60
```

`direct` pairs clients and sends private messages from one client to the other:

```bash
python3 tests/loadtest.py 127.0.0.1 6670 pass \
  --clients 500 --mode direct --rate 5000 --duration 60
```

## Important options

```text
--clients N                 Number of IRC connections
--connect-rate N            Connection attempts per second
--connect-concurrency N     Maximum simultaneous connection attempts
--mode fanout|direct        Message workload
--channel-size N            Users per channel in fan-out mode
--rate N                    Total PRIVMSG commands per second
--duration SECONDS          Traffic duration
--payload-size N            Generated message padding, up to 400 bytes
--slow-readers N            Clients that stop reading after setup
--probe-interval SECONDS    Interval between responsiveness PINGs
--settle SECONDS            Wait for final deliveries before reporting
--json PATH                 Write the result as JSON
--fail-on-errors            Exit nonzero for setup or delivery failures
```

Run `python3 tests/loadtest.py --help` for the complete command reference.

## Reading the report

- **Input** is the number and rate of `PRIVMSG` commands sent to the server.
- **Offered deliveries** includes channel fan-out and slow-reader destinations.
- **Readable deliveries** compares expected and received messages for clients
  that continue reading.
- **Integrity** reports sequence gaps, duplicates, or reordered messages.
- **Delivery latency** reports median, p95, p99, and maximum latency.
- **PING probes** show whether unrelated commands remain responsive under load.

In fan-out mode, one input message can create many output deliveries. For
example, one message to a 100-user channel creates 99 deliveries, so compare
both values when describing server capacity.

## Suggested test progression

Begin with 100 clients and increase gradually to 250, 500, 1,000, and beyond.
Raise the message rate separately so it is clear whether connections, parsing,
or output fan-out is the limiting factor.

For regression testing, add strict result checking and save the report:

```bash
python3 tests/loadtest.py 127.0.0.1 6670 pass \
  --clients 500 --channel-size 50 --rate 1000 --duration 60 \
  --fail-on-errors --json load-result.json
```

For trustworthy maximum-throughput measurements, run the tester on another
machine. When the tester and server share one machine, they compete for CPU and
the result measures their combined limit.
