#!/usr/bin/env python3
"""Black-box protocol and robustness tests for ft_irc.

The default invocation remains compatible with tests/quicktest.sh:

    python3 tests/harness.py [host] [port] [password]

Run the same suite against an ngIRCd instance as a behavioral baseline with:

    python3 tests/harness.py 127.0.0.1 6670 pass \
        --reference 127.0.0.1:6668 --reference-password pass

For deterministic reference runs, set ``MaxPenaltyTime = 0`` in ngIRCd's
``[Limits]`` section. Otherwise its flood penalties can deliberately delay replies
past the harness timeout.

The harness intentionally tests observable behavior only. Requirements such as using
one poll() loop and avoiding fork() still need source review during evaluation.
"""

import argparse
import errno
import os
import select
import socket
import sys
import time


DEFAULT_TIMEOUT = 0.8
IRC_MAX_LINE = 512


class IrcMessage(object):
    def __init__(self, raw):
        self.raw = raw
        self.prefix = ""
        self.command = ""
        self.params = []
        self._parse(raw)

    def _parse(self, raw):
        rest = raw
        if rest.startswith(":"):
            pieces = rest[1:].split(" ", 1)
            self.prefix = pieces[0]
            rest = pieces[1] if len(pieces) == 2 else ""
        if " :" in rest:
            middle, trailing = rest.split(" :", 1)
            tokens = middle.split()
            if tokens:
                self.command = tokens[0].upper()
                self.params = tokens[1:] + [trailing]
        else:
            tokens = rest.split()
            if tokens:
                self.command = tokens[0].upper()
                self.params = tokens[1:]

    def numeric(self):
        return self.command if len(self.command) == 3 and self.command.isdigit() else None

    def prefix_nick(self):
        return self.prefix.split("!", 1)[0]

    def __repr__(self):
        return self.raw


class Endpoint(object):
    def __init__(self, name, host, port, password):
        self.name = name
        self.host = host
        self.port = port
        self.password = password

    def __str__(self):
        return "{0} ({1}:{2})".format(self.name, self.host, self.port)


class Conn(object):
    """One IRC connection with strict CRLF framing and bounded waits."""

    def __init__(self, endpoint, name, timeout=DEFAULT_TIMEOUT):
        self.endpoint = endpoint
        self.name = name
        self.timeout = timeout
        self.sock = socket.create_connection(
            (endpoint.host, endpoint.port), timeout=timeout
        )
        self.sock.setblocking(False)
        self.buffer = b""
        self.closed = False
        self.bad_framing = False

    def send_raw(self, data):
        if isinstance(data, str):
            data = data.encode("utf-8")
        view = memoryview(data)
        deadline = time.monotonic() + self.timeout
        while view:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError("timed out writing to {0}".format(self.name))
            _, writable, _ = select.select([], [self.sock], [], remaining)
            if not writable:
                continue
            try:
                sent = self.sock.send(view)
            except OSError as exc:
                if exc.errno in (errno.EAGAIN, errno.EWOULDBLOCK, errno.EINTR):
                    continue
                raise
            if sent == 0:
                raise RuntimeError("connection closed while writing to {0}".format(self.name))
            view = view[sent:]

    def send(self, line):
        self.send_raw(line + "\r\n")

    def receive(self, timeout=None, idle=0.06):
        """Read complete IRC messages, stopping shortly after activity becomes idle."""
        timeout = self.timeout if timeout is None else timeout
        deadline = time.monotonic() + timeout
        saw_data = False
        while not self.closed:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            readable, _, _ = select.select([self.sock], [], [], remaining)
            if not readable:
                break
            try:
                chunk = self.sock.recv(65536)
            except OSError as exc:
                if exc.errno in (errno.EAGAIN, errno.EWOULDBLOCK, errno.EINTR):
                    continue
                self.closed = True
                break
            if not chunk:
                self.closed = True
                break
            self.buffer += chunk
            saw_data = True
            deadline = min(deadline, time.monotonic() + idle)

        messages = []
        while b"\r\n" in self.buffer:
            raw, self.buffer = self.buffer.split(b"\r\n", 1)
            messages.append(IrcMessage(raw.decode("utf-8", errors="replace")))

        if b"\n" in self.buffer:
            self.bad_framing = True
            chunks = self.buffer.split(b"\n")
            self.buffer = chunks[-1]
            for raw in chunks[:-1]:
                messages.append(IrcMessage(raw.rstrip(b"\r").decode("utf-8", errors="replace")))
        if not saw_data and self.closed:
            return messages
        return messages

    def drain(self):
        return self.receive(timeout=0.12, idle=0.03)

    def register(self, nick, user=None, realname=None, cap=False):
        user = user or nick
        realname = realname or (nick + " test user")
        lines = []
        if cap:
            lines.append("CAP LS 302")
        lines.extend([
            "PASS " + self.endpoint.password,
            "NICK " + nick,
            "USER {0} 0 * :{1}".format(user, realname),
        ])
        if cap:
            lines.append("CAP END")
        self.send_raw("\r\n".join(lines) + "\r\n")
        return self.receive()

    def close(self, abort=False):
        if self.sock is None:
            return
        try:
            if abort:
                self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass
        self.sock = None
        self.closed = True


def has_code(messages, code):
    code = str(code)
    return any(message.command == code for message in messages)


def find_message(messages, command, target=None, text=None, prefix_nick=None):
    command = command.upper()
    for message in messages:
        if message.command != command:
            continue
        if target is not None and (not message.params or message.params[0] != target):
            continue
        if text is not None and not any(text in param for param in message.params):
            continue
        if prefix_nick is not None and message.prefix_nick() != prefix_nick:
            continue
        return message
    return None


def dump(messages):
    if not messages:
        return "no messages"
    return " | ".join(repr(message.raw) for message in messages)


class Suite(object):
    def __init__(self, endpoint, timeout=DEFAULT_TIMEOUT, color=True, stress_clients=10):
        self.endpoint = endpoint
        self.timeout = timeout
        self.color = color
        self.stress_clients = stress_clients
        self.passed = 0
        self.failed = 0
        self.errors = 0
        self.connections = []
        self.serial = 0

    def token(self, stem="u"):
        self.serial += 1
        # Keep generated nicknames within the common nine-character limit.
        return "{0}{1:x}".format(stem[:3], self.serial)[-9:]

    def channel(self, stem="room"):
        return "#{0}{1:x}".format(stem[:8], self.serial + 1)

    def connect(self, name=None):
        conn = Conn(self.endpoint, name or self.token("c"), self.timeout)
        self.connections.append(conn)
        return conn

    def registered(self, stem="u", cap=False):
        nick = self.token(stem)
        conn = self.connect(nick)
        messages = conn.register(nick, cap=cap)
        if not has_code(messages, "001"):
            raise RuntimeError("registration for {0} failed: {1}".format(nick, dump(messages)))
        return conn, nick

    def check(self, label, condition, detail=""):
        if condition:
            self.passed += 1
            status = "\033[32mPASS\033[0m" if self.color else "PASS"
        else:
            self.failed += 1
            status = "\033[31mFAIL\033[0m" if self.color else "FAIL"
        suffix = " -> " + detail if detail and not condition else ""
        print("  {0} {1}{2}".format(status, label, suffix))

    def check_code(self, label, messages, code):
        self.check(label, has_code(messages, code), "expected {0}; got {1}".format(code, dump(messages)))

    def check_codes(self, label, messages, codes):
        self.check(
            label,
            any(has_code(messages, code) for code in codes),
            "expected one of {0}; got {1}".format("/".join(codes), dump(messages)),
        )

    def section(self, title):
        print("\n=== {0} ===".format(title))

    def cleanup(self):
        for conn in self.connections:
            conn.close()
        self.connections = []

    def run_case(self, title, function):
        self.section(title)
        try:
            function()
        except Exception as exc:
            self.errors += 1
            status = "\033[31mERROR\033[0m" if self.color else "ERROR"
            print("  {0} {1}: {2}".format(status, function.__name__, exc))
        finally:
            self.cleanup()

    def test_registration(self):
        c = self.connect("missing-pass")
        c.send("PASS")
        self.check_code("PASS without password -> 461", c.receive(), "461")

        c = self.connect("wrong-pass")
        c.send_raw("PASS incorrect\r\nNICK badpass\r\nUSER badpass 0 * :Bad Pass\r\n")
        messages = c.receive()
        self.check(
            "wrong password is rejected",
            has_code(messages, "464")
            or any(message.command == "ERROR" and "password" in message.raw.lower() for message in messages),
            dump(messages),
        )

        c = self.connect("missing-nick")
        c.send("PASS " + self.endpoint.password)
        c.drain()
        c.send("NICK")
        self.check_codes("NICK without nickname -> 431/461", c.receive(), ("431", "461"))
        c.send("USER too few")
        self.check_code("USER with too few parameters -> 461", c.receive(), "461")

        nick = self.token("reg")
        c = self.connect(nick)
        messages = c.register(nick, realname="Real Name With Spaces")
        for code in ("001", "002", "003", "004"):
            self.check_code("registration sends " + code, messages, code)
        self.check("server replies use CRLF", not c.bad_framing)
        self.check(
            "server replies stay within 512 bytes",
            all(len(message.raw.encode("utf-8")) + 2 <= IRC_MAX_LINE for message in messages),
            dump(messages),
        )

        c.send("PASS " + self.endpoint.password)
        self.check_code("PASS after registration -> 462", c.receive(), "462")
        c.send("USER again 0 * :Again")
        self.check_code("USER after registration -> 462", c.receive(), "462")
        c.send("DOESNOTEXIST value")
        self.check_code("unknown command -> 421", c.receive(), "421")

        duplicate = self.connect("duplicate")
        duplicate.send_raw(
            "PASS {0}\r\nNICK {1}\r\nUSER duplicate 0 * :Duplicate\r\n".format(
                self.endpoint.password, nick
            )
        )
        self.check_code("duplicate nickname -> 433", duplicate.receive(), "433")

        cap_nick = self.token("cap")
        cap = self.connect(cap_nick)
        cap_messages = cap.register(cap_nick, cap=True)
        self.check(
            "CAP LS is accepted during registration",
            find_message(cap_messages, "CAP") is not None,
            dump(cap_messages),
        )
        self.check_code("CAP handshake still completes registration", cap_messages, "001")

    def test_packet_aggregation(self):
        nick = self.token("pkt")
        c = self.connect(nick)
        c.send_raw("PA")
        premature = c.receive(timeout=0.15)
        self.check("partial command is not processed", not premature and not c.closed, dump(premature))
        c.send_raw("SS " + self.endpoint.password + "\r\nNI")
        c.send_raw("CK " + nick + "\r\nUSER " + nick)
        c.send_raw(" 0 * :Packet Aggregation\r\n")
        messages = c.receive()
        self.check_code("fragmented registration is reconstructed", messages, "001")

        c.send_raw("PING :one\r\nPING :two\r\n")
        messages = c.receive()
        self.check(
            "multiple commands in one packet are all processed",
            find_message(messages, "PONG", text="one") is not None
            and find_message(messages, "PONG", text="two") is not None,
            dump(messages),
        )

        c.send("ping :lowercase")
        messages = c.receive()
        self.check(
            "IRC command names are case-insensitive",
            find_message(messages, "PONG", text="lowercase") is not None,
            dump(messages),
        )

        hostile = self.connect("oversized-line")
        hostile.send_raw("X" * (IRC_MAX_LINE + 64))
        hostile.receive(timeout=0.15)
        probe, _ = self.registered("alive")
        probe.send("PING :after-oversized-input")
        messages = probe.receive()
        self.check(
            "oversized input cannot make the server unresponsive",
            find_message(messages, "PONG", text="after-oversized-input") is not None,
            dump(messages),
        )

    def test_join_and_messaging(self):
        a, anick = self.registered("anna")
        b, bnick = self.registered("ben")
        outsider, onick = self.registered("out")
        channel = self.channel("message")

        a.send("JOIN " + channel)
        messages = a.receive()
        self.check(
            "JOIN is echoed with the user prefix",
            find_message(messages, "JOIN", target=channel, prefix_nick=anick) is not None,
            dump(messages),
        )
        self.check_code("JOIN sends NAMES reply", messages, "353")
        self.check_code("JOIN sends end of NAMES", messages, "366")
        names = next((m for m in messages if m.command == "353"), None)
        self.check(
            "first channel member is marked operator in NAMES",
            names is not None and ("@" + anick) in names.params[-1].split(),
            dump(messages),
        )

        a.drain()
        b.send("JOIN " + channel)
        b_messages = b.receive()
        a_messages = a.receive()
        self.check(
            "joining user receives JOIN",
            find_message(b_messages, "JOIN", target=channel, prefix_nick=bnick) is not None,
            dump(b_messages),
        )
        self.check(
            "existing member sees JOIN",
            find_message(a_messages, "JOIN", target=channel, prefix_nick=bnick) is not None,
            dump(a_messages),
        )

        a.drain()
        b.drain()
        outsider.drain()
        payload = "channel message with spaces"
        a.send("PRIVMSG {0} :{1}".format(channel, payload))
        got_b = b.receive()
        got_a = a.receive(timeout=0.18)
        got_out = outsider.receive(timeout=0.18)
        self.check(
            "channel message reaches every other member",
            find_message(got_b, "PRIVMSG", target=channel, text=payload, prefix_nick=anick) is not None,
            dump(got_b),
        )
        self.check("channel message is not echoed to sender", not got_a, dump(got_a))
        self.check("channel message does not leak to outsiders", not got_out, dump(got_out))

        a.send("PRIVMSG {0} :direct message".format(bnick))
        messages = b.receive()
        self.check(
            "private message reaches nickname target",
            find_message(messages, "PRIVMSG", target=bnick, text="direct message", prefix_nick=anick)
            is not None,
            dump(messages),
        )

        outsider.send("PRIVMSG {0} :not a member".format(channel))
        outsider.receive()
        a.send("PRIVMSG")
        self.check_code("PRIVMSG without target -> 411", a.receive(), "411")
        a.send("PRIVMSG " + bnick)
        self.check_code("PRIVMSG without text -> 412", a.receive(), "412")
        a.send("PRIVMSG missingnick :hello")
        self.check_code("PRIVMSG to missing nickname -> 401", a.receive(), "401")
        a.send("PRIVMSG #missing-channel :hello")
        self.check_codes("PRIVMSG to missing channel -> 401/403", a.receive(), ("401", "403"))

        newnick = self.token("new")
        a.send("NICK " + newnick)
        own = a.receive()
        peer = b.receive()
        self.check(
            "nickname change is echoed to user",
            find_message(own, "NICK", target=newnick, prefix_nick=anick) is not None,
            dump(own),
        )
        self.check(
            "nickname change is broadcast to channel peers",
            find_message(peer, "NICK", target=newnick, prefix_nick=anick) is not None,
            dump(peer),
        )
        b.send("PRIVMSG {0} :after rename".format(newnick))
        messages = a.receive()
        self.check(
            "new nickname is immediately usable",
            find_message(messages, "PRIVMSG", target=newnick, text="after rename") is not None,
            dump(messages),
        )

    def test_topic_and_operator_mode(self):
        op, opnick = self.registered("top")
        member, membernick = self.registered("mem")
        channel = self.channel("topic")
        op.send("JOIN " + channel)
        op.receive()
        member.send("JOIN " + channel)
        member.receive()
        op.drain()

        member.send("TOPIC " + channel)
        self.check_code("empty topic query -> 331", member.receive(), "331")
        op.send("MODE {0} +t".format(channel))
        mode_messages = op.receive()
        member.receive()
        self.check(
            "operator can enable +t",
            find_message(mode_messages, "MODE", target=channel) is not None,
            dump(mode_messages),
        )
        member.send("TOPIC {0} :forbidden".format(channel))
        self.check_code("+t blocks a regular member -> 482", member.receive(), "482")

        topic = "submission topic with spaces"
        op.send("TOPIC {0} :{1}".format(channel, topic))
        own = op.receive()
        peer = member.receive()
        self.check(
            "TOPIC change is broadcast to setter",
            find_message(own, "TOPIC", target=channel, text=topic, prefix_nick=opnick) is not None,
            dump(own),
        )
        self.check(
            "TOPIC change is broadcast to channel peers",
            find_message(peer, "TOPIC", target=channel, text=topic, prefix_nick=opnick) is not None,
            dump(peer),
        )
        member.send("TOPIC " + channel)
        messages = member.receive()
        self.check(
            "TOPIC query returns current topic in 332",
            has_code(messages, "332") and any(topic in m.params for m in messages if m.command == "332"),
            dump(messages),
        )

        op.send("MODE {0} -t".format(channel))
        op.receive()
        member.receive()
        member.send("TOPIC {0} :member topic".format(channel))
        messages = member.receive()
        self.check(
            "-t lets a regular member change topic",
            find_message(messages, "TOPIC", target=channel, text="member topic", prefix_nick=membernick)
            is not None,
            dump(messages),
        )

        member.send("MODE {0} +i".format(channel))
        self.check_code("regular member cannot set channel modes -> 482", member.receive(), "482")
        op.send("MODE {0} +o {1}".format(channel, membernick))
        granted = member.receive()
        op.receive()
        self.check(
            "+o promotion is broadcast",
            find_message(granted, "MODE", target=channel) is not None,
            dump(granted),
        )
        member.send("MODE {0} +i".format(channel))
        messages = member.receive()
        op.receive()
        self.check(
            "promoted operator can change modes",
            find_message(messages, "MODE", target=channel) is not None,
            dump(messages),
        )
        op.send("MODE {0} -o {1}".format(channel, membernick))
        op.receive()
        member.receive()
        member.send("MODE {0} -i".format(channel))
        self.check_code("-o removes operator privileges", member.receive(), "482")

    def test_key_limit_and_invite_modes(self):
        op, _ = self.registered("key")
        second, _ = self.registered("sec")
        third, _ = self.registered("thi")
        fourth, fourthnick = self.registered("fou")
        channel = self.channel("modes")
        op.send("JOIN " + channel)
        op.receive()

        op.send("MODE {0} +k secret".format(channel))
        op.receive()
        second.send("JOIN " + channel)
        self.check_code("+k rejects a missing key -> 475", second.receive(), "475")
        second.send("JOIN {0} wrong".format(channel))
        self.check_code("+k rejects an incorrect key -> 475", second.receive(), "475")
        second.send("JOIN {0} secret".format(channel))
        messages = second.receive()
        self.check(
            "+k accepts the correct key",
            find_message(messages, "JOIN", target=channel) is not None,
            dump(messages),
        )
        op.receive()
        op.send("MODE {0} -k".format(channel))
        op.receive()
        second.receive()
        third.send("JOIN " + channel)
        messages = third.receive()
        self.check(
            "-k removes the channel key",
            find_message(messages, "JOIN", target=channel) is not None,
            dump(messages),
        )
        op.receive()
        second.receive()

        op.send("MODE {0} +l 3".format(channel))
        op.receive()
        second.receive()
        third.receive()
        fourth.send("JOIN " + channel)
        self.check_code("+l enforces the user limit -> 471", fourth.receive(), "471")
        op.send("MODE {0} -l".format(channel))
        op.receive()
        second.receive()
        third.receive()
        fourth.send("JOIN " + channel)
        messages = fourth.receive()
        self.check(
            "-l removes the user limit",
            find_message(messages, "JOIN", target=channel, prefix_nick=fourthnick) is not None,
            dump(messages),
        )

        invitee, inviteenick = self.registered("inv")
        blocked, _ = self.registered("blk")
        op.send("MODE {0} +i".format(channel))
        op.receive()
        second.receive()
        third.receive()
        fourth.receive()
        blocked.send("JOIN " + channel)
        self.check_code("+i blocks an uninvited user -> 473", blocked.receive(), "473")
        op.send("INVITE {0} {1}".format(inviteenick, channel))
        inviter_messages = op.receive()
        invitee_messages = invitee.receive()
        self.check_code("INVITE confirms with 341", inviter_messages, "341")
        self.check(
            "INVITE notifies its target",
            find_message(invitee_messages, "INVITE", target=inviteenick) is not None,
            dump(invitee_messages),
        )
        invitee.send("JOIN " + channel)
        messages = invitee.receive()
        self.check(
            "invited user can join +i channel",
            find_message(messages, "JOIN", target=channel) is not None,
            dump(messages),
        )
        op.receive()
        second.receive()
        third.receive()
        fourth.receive()
        op.send("MODE {0} -i".format(channel))
        op.receive()
        second.receive()
        third.receive()
        fourth.receive()
        invitee.receive()
        blocked.send("JOIN " + channel)
        messages = blocked.receive()
        self.check(
            "-i lets an uninvited user join",
            find_message(messages, "JOIN", target=channel) is not None,
            dump(messages),
        )

    def test_invite_and_kick_errors(self):
        op, opnick = self.registered("op")
        member, membernick = self.registered("mem")
        target, targetnick = self.registered("tar")
        outsider, _ = self.registered("out")
        channel = self.channel("kick")
        op.send("JOIN " + channel)
        op.receive()
        member.send("JOIN " + channel)
        member.receive()
        op.receive()

        op.send("MODE {0} +i".format(channel))
        op.receive()
        member.receive()

        member.send("INVITE {0} {1}".format(targetnick, channel))
        self.check_code("non-operator INVITE -> 482", member.receive(), "482")
        outsider.send("INVITE {0} {1}".format(targetnick, channel))
        self.check_code("INVITE while not on channel -> 442", outsider.receive(), "442")
        op.send("INVITE missingnick " + channel)
        self.check_code("INVITE missing nickname -> 401", op.receive(), "401")
        op.send("INVITE {0} {1}".format(membernick, channel))
        self.check_code("INVITE existing member -> 443", op.receive(), "443")

        member.send("KICK {0} {1} :not allowed".format(channel, opnick))
        self.check_code("non-operator KICK -> 482", member.receive(), "482")
        outsider.send("KICK {0} {1}".format(channel, membernick))
        self.check_code("KICK while not on channel -> 442", outsider.receive(), "442")
        op.send("KICK {0} missingnick".format(channel))
        self.check_codes("KICK missing user -> 401/441", op.receive(), ("401", "441"))

        op.send("KICK {0} {1} :evaluation kick".format(channel, membernick))
        own = op.receive()
        kicked = member.receive()
        self.check(
            "KICK is broadcast to operator",
            find_message(own, "KICK", target=channel, text="evaluation kick", prefix_nick=opnick)
            is not None,
            dump(own),
        )
        self.check(
            "KICK is delivered to removed user",
            find_message(kicked, "KICK", target=channel, prefix_nick=opnick) is not None,
            dump(kicked),
        )
        op.send("PRIVMSG {0} :after kick".format(channel))
        messages = member.receive(timeout=0.18)
        self.check("kicked user receives no later channel messages", not messages, dump(messages))

    def test_quit_and_disconnect_cleanup(self):
        watcher, _ = self.registered("wat")
        quitter, quitnick = self.registered("qui")
        channel = self.channel("quit")
        watcher.send("JOIN " + channel)
        watcher.receive()
        quitter.send("JOIN " + channel)
        quitter.receive()
        watcher.receive()

        quitter.send("QUIT :planned departure")
        messages = watcher.receive()
        self.check(
            "QUIT is broadcast with its reason",
            find_message(messages, "QUIT", text="planned departure", prefix_nick=quitnick) is not None,
            dump(messages),
        )

        abrupt, abruptnick = self.registered("drop")
        abrupt.send("JOIN " + channel)
        abrupt.receive()
        watcher.receive()
        abrupt.close(abort=True)
        messages = watcher.receive(timeout=max(1.0, self.timeout))
        self.check(
            "abrupt disconnect is broadcast to channel peers",
            find_message(messages, "QUIT", prefix_nick=abruptnick) is not None,
            dump(messages),
        )

        reuse = self.connect("nickname-reuse")
        messages = reuse.register(abruptnick)
        if not has_code(messages, "001"):
            time.sleep(0.08)
            messages += reuse.receive()
        self.check_code("disconnected nickname can be reused", messages, "001")

        watcher.send("PING :still-responsive")
        messages = watcher.receive()
        self.check(
            "remaining client stays responsive after disconnects",
            find_message(messages, "PONG", text="still-responsive") is not None,
            dump(messages),
        )

    def test_concurrency_and_backpressure(self):
        clients = []
        channel = self.channel("stress")
        for index in range(self.stress_clients):
            nick = self.token("s")
            conn = self.connect("stress-" + str(index))
            conn.send_raw(
                "PASS {0}\r\nNICK {1}\r\nUSER {1} 0 * :Stress {2}\r\n".format(
                    self.endpoint.password, nick, index
                )
            )
            clients.append((conn, nick))
        registrations = [conn.receive() for conn, _ in clients]
        self.check(
            "multiple simultaneous clients all register",
            all(has_code(messages, "001") for messages in registrations),
            " | ".join(dump(messages) for messages in registrations if not has_code(messages, "001")),
        )

        for conn, _ in clients:
            conn.send("JOIN " + channel)
        joins = [conn.receive() for conn, _ in clients]
        self.check(
            "multiple clients can join one channel",
            all(find_message(messages, "JOIN", target=channel) is not None for messages in joins),
            "one or more JOIN replies were missing",
        )
        for conn, _ in clients:
            conn.drain()

        sender, sender_nick = clients[0]
        marker = "fanout-{0}".format(self.serial)
        sender.send("PRIVMSG {0} :{1}".format(channel, marker))
        deliveries = [conn.receive() for conn, _ in clients[1:]]
        self.check(
            "channel message fans out to all peers",
            all(
                find_message(messages, "PRIVMSG", target=channel, text=marker, prefix_nick=sender_nick)
                is not None
                for messages in deliveries
            ),
            "received by {0}/{1} peers".format(
                sum(find_message(messages, "PRIVMSG", target=channel, text=marker) is not None for messages in deliveries),
                len(deliveries),
            ),
        )

        slow, slow_nick = self.registered("slow")
        probe, _ = self.registered("probe")
        payload = "x" * 400
        burst = "".join(
            "PRIVMSG {0} :{1}{2:04d}\r\n".format(slow_nick, payload, index)
            for index in range(700)
        )
        sender.send_raw(burst)
        probe.send("PING :during-slow-reader")
        messages = probe.receive(timeout=max(1.5, self.timeout))
        self.check(
            "a slow reader does not block unrelated clients",
            find_message(messages, "PONG", text="during-slow-reader") is not None,
            dump(messages),
        )

    def run(self):
        print("\nTesting {0} with password {1!r}".format(self.endpoint, self.endpoint.password))
        cases = (
            ("Registration and client handshake", self.test_registration),
            ("Packet aggregation and parsing", self.test_packet_aggregation),
            ("JOIN, NAMES, PRIVMSG, and NICK", self.test_join_and_messaging),
            ("TOPIC and operator mode", self.test_topic_and_operator_mode),
            ("Channel key, limit, and invite modes", self.test_key_limit_and_invite_modes),
            ("INVITE and KICK permissions", self.test_invite_and_kick_errors),
            ("QUIT and disconnect cleanup", self.test_quit_and_disconnect_cleanup),
            ("Concurrency and slow-reader isolation", self.test_concurrency_and_backpressure),
        )
        for title, function in cases:
            self.run_case(title, function)
        total = self.passed + self.failed
        print("\n" + "=" * 64)
        print(
            "{0}: {1} passed, {2} failed, {3} scenario errors ({4} checks)".format(
                self.endpoint.name, self.passed, self.failed, self.errors, total
            )
        )
        return self.failed + self.errors


def parse_endpoint(value):
    if value.count(":") != 1:
        raise argparse.ArgumentTypeError("endpoint must have the form HOST:PORT")
    host, port_text = value.rsplit(":", 1)
    try:
        port = int(port_text)
    except ValueError:
        raise argparse.ArgumentTypeError("endpoint port must be an integer")
    if not host or port < 1 or port > 65535:
        raise argparse.ArgumentTypeError("invalid endpoint")
    return host, port


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", nargs="?", default="127.0.0.1")
    parser.add_argument("port", nargs="?", type=int, default=6667)
    parser.add_argument("password", nargs="?", default="testpass")
    parser.add_argument(
        "--reference",
        type=parse_endpoint,
        metavar="HOST:PORT",
        help="also run the complete suite against an ngIRCd reference server",
    )
    parser.add_argument(
        "--reference-password",
        help="reference password (defaults to the target password)",
    )
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    parser.add_argument("--stress-clients", type=int, default=10)
    parser.add_argument("--no-color", action="store_true")
    args = parser.parse_args()
    if args.port < 1 or args.port > 65535:
        parser.error("port must be between 1 and 65535")
    if args.timeout <= 0:
        parser.error("timeout must be positive")
    if args.stress_clients < 2 or args.stress_clients > 100:
        parser.error("--stress-clients must be between 2 and 100")
    return args


def main():
    args = parse_args()
    color = not args.no_color and sys.stdout.isatty() and os.environ.get("NO_COLOR") is None
    endpoints = [Endpoint("target", args.host, args.port, args.password)]
    if args.reference:
        endpoints.append(
            Endpoint(
                "ngIRCd reference",
                args.reference[0],
                args.reference[1],
                args.reference_password or args.password,
            )
        )

    failures = 0
    for endpoint in endpoints:
        try:
            failures += Suite(
                endpoint,
                timeout=args.timeout,
                color=color,
                stress_clients=args.stress_clients,
            ).run()
        except (OSError, RuntimeError) as exc:
            failures += 1
            print("\nERROR: cannot test {0}: {1}".format(endpoint, exc), file=sys.stderr)
    return min(failures, 255)


if __name__ == "__main__":
    sys.exit(main())
