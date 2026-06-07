# ft_irc Subject Reference

Source: `en.subject.pdf`  
Project: `ft_irc` / Internet Relay Chat  
Subject version: 10.0

This document is a condensed, AI-agent-friendly reference extracted from the official
subject PDF. It preserves implementation requirements, allowed functions, evaluation
constraints, README requirements, and bonus expectations.

## Project Summary

Build an IRC server in C++98.

The project is about understanding Internet protocols by implementing a server that an
actual IRC client can connect to and use. You will not build an IRC client, and you will
not implement server-to-server communication.

IRC servers can be connected together in real IRC networks, but that is explicitly out
of scope for this project.

## General Rules

- The program must not crash, even in edge cases such as memory exhaustion.
- The program must not quit unexpectedly.
- If the program crashes or behaves as non-functional, the grade is `0`.
- A `Makefile` is required and must compile the source files.
- The `Makefile` must not perform unnecessary relinking.
- Required `Makefile` rules:
  - `$(NAME)`
  - `all`
  - `clean`
  - `fclean`
  - `re`
- Compile with `c++` using:

```sh
-Wall -Wextra -Werror
```

- The code must comply with the C++98 standard.
- It must still compile when adding:

```sh
-std=c++98
```

- Prefer C++ headers and facilities when available.
  - Example: prefer `<cstring>` over `<string.h>`.
- C functions are allowed when necessary, but C++ versions should be preferred when
  possible.
- External libraries are forbidden.
- Boost is forbidden.

## Mandatory Part

### Program Metadata

| Field | Requirement |
| --- | --- |
| Program name | `ircserv` |
| Submitted files | `Makefile`, `*.h`, `*.hpp`, `*.cpp`, `*.tpp`, `*.ipp`, optional configuration file |

### Execution

The executable must be run as:

```sh
./ircserv <port> <password>
```

Arguments:

- `port`: port number where the IRC server listens for incoming IRC connections.
- `password`: connection password required by IRC clients connecting to the server.

### Explicit Non-Goals

- Do not develop an IRC client.
- Do not implement server-to-server communication.

### Allowed External Functions

Everything available in C++98 is allowed, plus the following external/system functions:

```txt
socket
close
setsockopt
getsockname
getprotobyname
gethostbyname
getaddrinfo
freeaddrinfo
bind
connect
listen
accept
htons
htonl
ntohs
ntohl
inet_addr
inet_ntoa
inet_ntop
send
recv
signal
sigaction
sigemptyset
sigfillset
sigaddset
sigdelset
sigismember
lseek
fstat
fcntl
poll
```

The subject also allows an equivalent to `poll()`, such as `select()`, `kqueue()`, or
`epoll()`.

## Mandatory Requirements

### Concurrency and I/O

- The server must handle multiple clients simultaneously.
- The server must not hang.
- Forking is prohibited.
- All I/O operations must be non-blocking.
- Only one `poll()` call, or one equivalent event mechanism, may be used to handle all
  I/O operations:
  - reading
  - writing
  - listening
  - accepting
  - related socket activity

Important evaluation rule:

If the server attempts to `read`/`recv` or `write`/`send` on any file descriptor without
using `poll()` or an equivalent event mechanism, the grade is `0`.

The subject notes that non-blocking file descriptors technically make direct
`read`/`recv` or `write`/`send` calls non-blocking, but doing this outside the event
mechanism wastes resources and is forbidden for this project.

### Networking

- Client/server communication must use TCP/IP.
- IPv4 or IPv6 is allowed.

### Reference Client

- Choose an existing IRC client as the reference client.
- The reference client will be used during evaluation.
- The reference client must connect to the server without errors.
- Using the reference client with your server should resemble using it with an official
  IRC server, within the required feature scope.

### Required IRC Features

The server must support the following through the reference client:

- Authenticate with the server.
- Set a nickname.
- Set a username.
- Join a channel.
- Send and receive private messages.
- Send messages to a channel.
- Forward every message sent to a channel to every other client in that channel.
- Maintain channel operators and regular users.

### Required Channel Operator Commands

Implement these channel-operator-specific commands:

- `KICK`: eject a client from a channel.
- `INVITE`: invite a client to a channel.
- `TOPIC`: change or view the channel topic.
- `MODE`: change channel modes.

Required `MODE` flags:

- `i`: set/remove invite-only mode.
- `t`: set/remove restriction of `TOPIC` to channel operators.
- `k`: set/remove the channel key/password.
- `o`: give/take channel operator privilege.
- `l`: set/remove the user limit.

Clean code is expected.

## macOS-Specific Rule

macOS handles `write()` differently from other Unix systems. On macOS, `fcntl()` is
permitted only to set file descriptors to non-blocking mode:

```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

Any other `fcntl()` flag is forbidden under this macOS-specific allowance.

## Packet Aggregation Test

The server must correctly handle partial data, low bandwidth, and split commands.

Example test with `nc`:

```sh
nc -C 127.0.0.1 6667
com^Dman^Dd
```

Use `Ctrl+D` to send a command in several parts:

- `com`
- `man`
- `d\n`

The server must aggregate received packets and rebuild the full command before
processing it.

## README Requirements

A `README.md` must be provided at the root of the Git repository.

Purpose: allow peers, staff, recruiters, or anyone unfamiliar with the project to
quickly understand what the project is, how to run it, and where to find more
information.

The README must be written in English.

The first line must be italicized and must read:

```md
*This project has been created as part of the 42 curriculum by <login1>[, <login2>[, <login3>[...]]].*
```

Required sections:

- `Description`: clear presentation of the project, its goal, and a brief overview.
- `Instructions`: relevant compilation, installation, and/or execution instructions.
- `Resources`: classic references such as documentation, articles, and tutorials.
- AI usage description:
  - what AI was used for
  - which tasks it supported
  - which parts of the project it affected

Additional sections may be appropriate, such as usage examples, feature list, protocol
notes, testing notes, or technical choices.

## Bonus Part

Optional bonus features:

- File transfer.
- A bot.

Bonus work is evaluated only if the mandatory part is perfect. "Perfect" means the
mandatory part is complete and works without malfunctioning. If any mandatory
requirement is missing or failing, the bonus part is not evaluated.

## Submission and Evaluation

- Submit the assignment through the Git repository as usual.
- Only the work inside the repository is evaluated during defense.
- Double-check submitted filenames.
- Test programs are encouraged, even though they are not submitted or graded.
- You may use any useful tests during evaluation.
- The chosen reference IRC client will be used during evaluation.

During evaluation, a brief project modification may be requested if mentioned in the
evaluation guidelines. This may involve:

- a minor behavior change
- a few lines of code to write or rewrite
- an easy-to-add feature
- a small update to a function or script
- a display change
- a data-structure adjustment

The purpose is to verify actual understanding of a specific part of the project. The
scope and target are defined by the evaluation guidelines and may vary between
evaluations.

