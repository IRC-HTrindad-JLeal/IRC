# IRC Reference Library

Working reference for building `ft_irc`. Keep this file practical: add links,
notes, command examples, client quirks, and implementation decisions as the
project grows.

## Project Context

- Local subject reference: [`irc.subject.md`](./subj/irc.subject.md)
- Goal: implement a single IRC server in C++98.
- Out of scope: IRC client, server-to-server protocol, services such as
  NickServ/ChanServ, TLS, SASL, and general-purpose IRC network federation.
- Hard constraint: all socket I/O must be non-blocking and go through one
  `poll()` loop, or an equivalent event mechanism.

## Source Priority

Use sources in this order when behavior is unclear:

1. The 42 subject, because it defines what is graded.
2. RFC 2812, because it describes the client-to-server IRC protocol.
3. RFC 1459, because it is the original IRC protocol reference.
4. Modern IRC / IRCv3 docs, because real clients may send modern compatibility
   commands such as `CAP`.
5. Reference-client documentation and observed behavior.

## Core Protocol Sources

- RFC 2812: IRC Client Protocol  
  <https://datatracker.ietf.org/doc/html/rfc2812>
- RFC 1459: Original IRC Protocol  
  <https://www.rfc-editor.org/rfc/rfc1459>
- RFC 2811: IRC Channel Management  
  <https://datatracker.ietf.org/doc/html/rfc2811>
- Modern IRC Client Protocol  
  <https://modern.ircdocs.horse/>
- IRCv3 Capability Negotiation  
  <https://ircv3.net/specs/extensions/capability-negotiation.html>
- IRCv3 Message Tags  
  <https://ircv3.net/specs/extensions/message-tags.html>

## Networking Sources

- `poll(2)` Linux manual page  
  <https://man7.org/linux/man-pages/man2/poll.2.html>
- `fcntl(2)` Linux manual page  
  <https://man7.org/linux/man-pages/man2/fcntl.2.html>
- Beej's Guide to Network Programming  
  <https://beej.us/guide/bgnet/html/>

## Reference Client Sources

- WeeChat user guide  
  <https://weechat.org/files/doc/stable/weechat_user.en.html>
- WeeChat quick start  
  <https://weechat.org/files/doc/weechat/stable/weechat_quickstart.en.html>
- Irssi documentation  
  <https://irssi.org/documentation/>

## Useful Terms

### IRC Server

The program clients connect to. In this project, the server accepts TCP
connections, registers users, tracks channels, and routes messages. It does not
connect to other IRC servers.

### Client

A connected user socket plus IRC state: password status, nickname, username,
registration status, receive buffer, send queue, and joined channels.

### Registration

The process where a connection becomes a known IRC user. For this project, the
important commands are:

```txt
PASS <password>
NICK <nickname>
USER <username> 0 * :<realname>
```

RFC 2812 recommends `PASS`, then `NICK`, then `USER`. After successful
registration, the server should send welcome numerics such as `001`, `002`,
`003`, and `004`.

### Numeric Reply

Server replies often use three-digit command numbers. Examples:

```txt
:ircserv 001 alice :Welcome to the Internet Relay Network alice
:ircserv 433 * alice :Nickname is already in use
```

Keep a small numeric table in code or docs as commands are implemented.

### Message Line

IRC is line-based. A message ends with CRLF:

```txt
COMMAND param1 param2 :trailing text\r\n
```

Important implementation detail: TCP is a stream. A single `recv()` may return
half a command, one command, or multiple commands. Each client needs an input
buffer, and the parser should only process complete lines.

### Prefix / Source

Server-to-client messages often include the sender before the command:

```txt
:alice!alice@localhost PRIVMSG #chat :hello
```

Clients should not send their own prefix; the server decides the true source.

### Trailing Parameter

The part after `:` is one parameter even if it contains spaces:

```txt
PRIVMSG #chat :hello there everyone
```

Here, the parameters are `#chat` and `hello there everyone`.

### Channel

A named chat room, usually beginning with `#`. The server tracks members,
operators, topic, key/password, invite list, user limit, and mode flags.

### Channel Operator

A channel member with elevated permissions. The first user to create/join a new
channel is normally made an operator. Required operator-only commands/modes:

- `KICK`: remove a user from a channel.
- `INVITE`: invite a user to a channel.
- `TOPIC`: change topic when topic changes are operator-restricted.
- `MODE +i/-i`: enable/disable invite-only channel.
- `MODE +t/-t`: restrict/unrestrict topic changes to operators.
- `MODE +k/-k`: set/remove channel key.
- `MODE +o/-o`: give/take channel operator status.
- `MODE +l/-l`: set/remove user limit.

### CAP

IRCv3 capability negotiation. Modern clients often send this before completing
registration:

```txt
CAP LS 302
```

For a minimal server, it is usually enough to reply with no capabilities and
handle `CAP END` so the client can continue registration:

```txt
:ircserv CAP * LS :
```

### PING / PONG

Used to keep connections alive and prove the other side is responsive.

```txt
PING :ircserv
PONG :ircserv
```

Many real clients expect this to work.

## Mandatory Command Checklist

Registration and connection:

- `PASS`
- `NICK`
- `USER`
- `PING`
- `PONG`
- `QUIT`
- `CAP` minimal compatibility

Channel basics:

- `JOIN`
- `PART`
- `PRIVMSG`
- `NOTICE` optional but useful
- `NAMES` useful for client compatibility
- `TOPIC`

Operator/channel modes:

- `KICK`
- `INVITE`
- `MODE`

Helpful but not mandatory:

- `WHO`
- `WHOIS`
- `MOTD`
- `LIST`

## Implementation Notes

### One `poll()` Loop

The subject is strict: do not call `recv()` or `send()` just because an fd
exists. Wait for `poll()` to report the fd as readable or writable, then do the
operation. Keep outgoing messages in a per-client queue and enable `POLLOUT`
only when that queue is non-empty.

### Non-Blocking Sockets

Set the listening socket and accepted client sockets to non-blocking mode:

```cpp
fcntl(fd, F_SETFL, O_NONBLOCK);
```

On macOS, the subject specifically allows `fcntl()` only for setting
non-blocking mode.

### Receive Buffer

Each client should have a string buffer:

1. Append bytes from `recv()`.
2. Search for `\r\n` or, for practical `nc` testing, possibly tolerate `\n`.
3. Extract one complete line.
4. Parse and dispatch the command.
5. Leave incomplete bytes in the buffer for the next `recv()`.

### Send Queue

Do not assume one `send()` writes the whole reply. Store pending output and trim
only the bytes actually sent.

### Case Mapping

IRC nicknames and channel names are case-insensitive in practice. At minimum,
avoid allowing both `Alice` and `alice` as separate users. RFC-style case mapping
also treats some bracket characters specially, but a simple lowercase helper is
often enough for a first version unless evaluation catches it.

## Raw Protocol Examples

Register with `nc -C`:

```txt
PASS password
NICK alice
USER alice 0 * :Alice Example
```

Join a channel:

```txt
JOIN #chat
```

Send a channel message:

```txt
PRIVMSG #chat :hello channel
```

Send a private message:

```txt
PRIVMSG bob :hello bob
```

Set topic:

```txt
TOPIC #chat :Project discussion
```

Set modes:

```txt
MODE #chat +i
MODE #chat +k secret
MODE #chat +l 10
MODE #chat +o bob
```

Kick a user:

```txt
KICK #chat bob :reason
```

## Testing Ideas

- Build with `make`.
- Run `./ircserv 6667 password`.
- Connect with `nc -C 127.0.0.1 6667`.
- Connect two clients and verify channel message forwarding.
- Send partial commands using the subject's packet aggregation test.
- Try duplicate nicknames.
- Try wrong password.
- Try joining invite-only channels.
- Try channel key and user limit behavior.
- Try operator-only commands as a regular user and verify errors.
- Connect with WeeChat or Irssi and record any extra commands they send.

## Open Questions To Resolve Later

- Which reference client will be used for evaluation practice: WeeChat or Irssi?
- Should the parser accept LF-only lines from `nc`, or strictly require CRLF?
- Which numerics should be implemented for best client compatibility beyond the
  mandatory commands?
- What exact server name should be used in prefixes and welcome replies?
- Should channel and nick comparison implement full RFC1459 case mapping or a
  simpler lowercase-only mapping?

