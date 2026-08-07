#!/usr/bin/env python3
# IRC server test harness for ft_irc.
#
# Usage:
#   ./ircserv 6667 testpass &
#   python3 tests/harness.py [host] [port] [password]
#
# Drives multiple real client connections through the server and checks the
# replies. Each check prints PASS/FAIL; a FAIL shows what was expected vs. what
# the server actually sent, so you can pinpoint the bug. Exit code is the number
# of failed checks (0 == all good).

import socket
import sys
import time

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 6667
PASSWORD = sys.argv[3] if len(sys.argv) > 3 else "testpass"

# ---------------------------------------------------------------- result tally
PASSED = 0
FAILED = 0


def check(label, ok, detail=""):
    global PASSED, FAILED
    if ok:
        PASSED += 1
        print("  \033[32mPASS\033[0m " + label)
    else:
        FAILED += 1
        print("  \033[31mFAIL\033[0m " + label + ("  -> " + detail if detail else ""))


def section(title):
    print("\n=== " + title + " ===")


# ---------------------------------------------------------------- connection
class Conn:
    """One client connection with a line-buffered, non-blocking reader."""

    def __init__(self, name):
        self.name = name
        self.sock = socket.create_connection((HOST, PORT))
        self.sock.setblocking(False)
        self.buf = ""

    def send(self, line):
        self.sock.sendall((line + "\r\n").encode())
        time.sleep(0.05)

    def _drain(self, wait=0.35):
        """Pull everything available within `wait` seconds into self.buf."""
        end = time.time() + wait
        while time.time() < end:
            try:
                data = self.sock.recv(4096)
                if not data:
                    break
                self.buf += data.decode(errors="replace")
                end = time.time() + 0.15  # extend a little on activity
            except BlockingIOError:
                time.sleep(0.03)
            except (ConnectionResetError, OSError):
                break

    def lines(self, wait=0.35):
        """Return complete lines received so far and consume them."""
        self._drain(wait)
        parts = self.buf.split("\r\n")
        self.buf = parts[-1]
        return [p for p in parts[:-1] if p]

    def expect(self, wait=0.5):
        """Convenience: join all lines into one blob for substring checks."""
        return "\n".join(self.lines(wait))

    def register(self, nick, user=None, realname=None):
        user = user or nick
        realname = realname or nick
        self.send("PASS " + PASSWORD)
        self.send("NICK " + nick)
        self.send("USER {0} 0 * :{1}".format(user, realname))
        return self.expect()

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def has_code(blob, code):
    """True if a numeric reply with the given 3-digit code appears."""
    for ln in blob.split("\n"):
        toks = ln.split()
        if len(toks) >= 2 and toks[1] == code:
            return True
    return False


# ============================================================== SCENARIOS

def test_registration():
    section("Registration")

    # wrong password
    c = Conn("wrongpass")
    c.send("PASS wrongpassword")
    c.send("NICK bob")
    c.send("USER bob 0 * :Bob")
    blob = c.expect()
    check("wrong password rejected (464)", has_code(blob, "464"), repr(blob))
    c.close()

    # clean registration
    c = Conn("alice")
    blob = c.register("alice")
    check("valid registration -> 001 welcome", has_code(blob, "001"), repr(blob))
    check("registration sends 004 (MYINFO)", has_code(blob, "004"), repr(blob))
    c.close()

    # NICK collision
    a = Conn("a"); a.register("carol")
    b = Conn("b")
    b.send("PASS " + PASSWORD)
    b.send("NICK carol")
    blob = b.expect()
    check("duplicate nick rejected (433)", has_code(blob, "433"), repr(blob))
    a.close(); b.close()

    # command before registration
    c = Conn("early")
    c.send("JOIN #chan")
    blob = c.expect()
    check("command before registration rejected (451)", has_code(blob, "451"), repr(blob))
    c.close()

    # unknown command
    c = Conn("unknown"); c.register("dave")
    c.send("FOOBAR baz")
    blob = c.expect()
    check("unknown command rejected (421)", has_code(blob, "421"), repr(blob))
    c.close()


def test_join_privmsg():
    section("JOIN / PRIVMSG")

    a = Conn("a"); a.register("anna")
    b = Conn("b"); b.register("ben")

    a.send("JOIN #room")
    blob = a.expect()
    check("JOIN echoes JOIN line", "JOIN" in blob and "#room" in blob, repr(blob))
    check("JOIN sends names reply (353)", has_code(blob, "353"), repr(blob))
    check("JOIN sends end of names (366)", has_code(blob, "366"), repr(blob))

    # second client joins -> first client should be notified
    a.lines(0.2)  # clear
    b.send("JOIN #room")
    a_notice = a.expect()
    check("existing member notified of new JOIN", "ben" in a_notice and "JOIN" in a_notice, repr(a_notice))

    # PRIVMSG to channel relays to other member, not to self
    a.lines(0.2); b.lines(0.2)
    a.send("PRIVMSG #room :hello room")
    got_b = b.expect()
    got_a = a.expect(0.2)
    check("PRIVMSG to channel reaches other member", "hello room" in got_b, repr(got_b))
    check("PRIVMSG to channel not echoed to sender", "hello room" not in got_a, repr(got_a))

    # PRIVMSG to nick
    b.lines(0.2)
    a.send("PRIVMSG ben :direct hi")
    got_b = b.expect()
    check("PRIVMSG to nick delivered", "direct hi" in got_b, repr(got_b))

    # PRIVMSG to nonexistent nick
    a.send("PRIVMSG ghost :hi")
    blob = a.expect()
    check("PRIVMSG to missing nick -> 401", has_code(blob, "401"), repr(blob))

    # PRIVMSG to nonexistent channel
    a.send("PRIVMSG #nope :hi")
    blob = a.expect()
    check("PRIVMSG to missing channel -> 403/404", has_code(blob, "403") or has_code(blob, "404"), repr(blob))

    a.close(); b.close()


def test_topic():
    section("TOPIC")
    a = Conn("a"); a.register("tom")
    a.send("JOIN #topictest"); a.lines(0.2)

    a.send("TOPIC #topictest")
    blob = a.expect()
    check("TOPIC query on empty -> 331 (no topic)", has_code(blob, "331") or has_code(blob, "332"), repr(blob))

    a.send("TOPIC #topictest :new subject")
    blob = a.expect()
    check("TOPIC set echoes 332 or TOPIC line", has_code(blob, "332") or "TOPIC" in blob, repr(blob))

    a.send("TOPIC #topictest")
    blob = a.expect()
    check("TOPIC query returns set topic (332)", "new subject" in blob, repr(blob))
    a.close()


def test_mode():
    section("MODE")
    a = Conn("a"); a.register("op")
    b = Conn("b"); b.register("peon")
    a.send("JOIN #modetest"); a.lines(0.2)

    a.send("MODE #modetest +t")
    blob = a.expect()
    check("MODE +t acknowledged (MODE line or no error)", "MODE" in blob or blob == "", repr(blob))

    # +k key then a non-member tries to join without key
    a.send("MODE #modetest +k s3cret"); a.lines(0.2)
    b.send("JOIN #modetest")
    blob = b.expect()
    check("MODE +k blocks join without key (475)", has_code(blob, "475"), repr(blob))

    # join with key
    b.send("JOIN #modetest s3cret")
    blob = b.expect()
    check("join with correct key succeeds", "JOIN" in blob and "#modetest" in blob, repr(blob))

    # +l user limit
    a.send("MODE #modetest +l 1"); a.lines(0.2)
    c = Conn("c"); c.register("third")
    c.send("JOIN #modetest s3cret")
    blob = c.expect()
    check("MODE +l enforces user limit (471)", has_code(blob, "471"), repr(blob))
    c.close()

    a.close(); b.close()

    # +i invite-only, isolated in its own fresh channel
    e = Conn("e"); e.register("keeper"); e.lines(0.4)
    e.send("JOIN #invonly"); e.lines(0.4)
    e.send("MODE #invonly +i"); e.lines(0.4)
    d = Conn("d"); d.register("outsider"); d.lines(0.4)
    d.send("JOIN #invonly")
    blob = d.expect(0.6)
    check("MODE +i blocks uninvited join (473)", has_code(blob, "473"), repr(blob))
    d.close(); e.close()


def test_invite():
    section("INVITE")
    a = Conn("a"); a.register("host")
    b = Conn("b"); b.register("guest")
    a.send("JOIN #invtest"); a.lines(0.2)
    a.send("MODE #invtest +i"); a.lines(0.2)

    a.send("INVITE guest #invtest")
    blob_a = a.expect()
    blob_b = b.expect()
    check("INVITE gives inviter 341 confirmation", has_code(blob_a, "341"), repr(blob_a))
    check("INVITE notifies target", "invtest" in blob_b.lower(), repr(blob_b))

    b.send("JOIN #invtest")
    blob = b.expect()
    check("invited user can join +i channel", "JOIN" in blob and "#invtest" in blob, repr(blob))
    a.close(); b.close()


def test_kick():
    section("KICK")
    a = Conn("a"); a.register("chief")
    b = Conn("b"); b.register("target")
    a.send("JOIN #kicktest"); a.lines(0.2)
    b.send("JOIN #kicktest"); a.lines(0.2); b.lines(0.2)

    a.send("KICK #kicktest target :bye")
    blob_b = b.expect()
    check("KICK notifies the kicked user", "KICK" in blob_b and "target" in blob_b, repr(blob_b))

    # non-operator cannot kick
    c = Conn("c"); c.register("rando")
    c.send("JOIN #kicktest"); c.lines(0.2)
    c.send("KICK #kicktest chief :nope")
    blob = c.expect()
    check("non-operator KICK rejected (482)", has_code(blob, "482"), repr(blob))
    c.close()
    a.close(); b.close()


def test_ping():
    section("PING / QUIT")
    a = Conn("a"); a.register("pinger")
    a.send("PING :token123")
    blob = a.expect()
    check("PING answered with PONG token", "PONG" in blob and "token123" in blob, repr(blob))

    b = Conn("b"); b.register("quitter")
    b.send("JOIN #quittest"); b.lines(0.2)
    a.send("JOIN #quittest"); a.lines(0.2)
    b.send("QUIT :leaving")
    blob = a.expect()
    check("QUIT broadcast to channel members", "QUIT" in blob and "quitter" in blob, repr(blob))
    a.close(); b.close()


def main():
    print("Testing server at {0}:{1} (password: {2})".format(HOST, PORT, PASSWORD))
    for fn in (test_registration, test_join_privmsg, test_topic,
               test_mode, test_invite, test_kick, test_ping):
        try:
            fn()
        except Exception as e:  # keep going even if one scenario blows up
            global FAILED
            FAILED += 1
            print("  \033[31mERROR\033[0m in {0}: {1}".format(fn.__name__, e))
    print("\n" + "=" * 40)
    print("RESULT: {0} passed, {1} failed".format(PASSED, FAILED))
    sys.exit(FAILED)


if __name__ == "__main__":
    main()
