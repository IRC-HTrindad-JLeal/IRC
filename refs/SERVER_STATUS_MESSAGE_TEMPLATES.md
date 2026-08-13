# Server Status Message Templates

This reference contains **server-terminal status messages**, not IRC protocol
replies sent to clients. The wording follows the output observed from ngIRCd
26.1 running in foreground mode (`ngircd -n`).

Values inside angle brackets are placeholders. Do not print the angle brackets.

## Recommended log prefix

ngIRCd prefixes each terminal message like this:

```text
[<pid>:<level> <elapsed-seconds>] <message>
```

Example:

```text
[311359:6   27] Accepted connection 7 from "127.0.0.1:48790" on socket 6.
```

Placeholder meanings:

| Placeholder | Meaning | Typical source |
|---|---|---|
| `<pid>` | Server process ID | `getpid()` |
| `<level>` | Numeric syslog-style severity | See the table below |
| `<elapsed-seconds>` | Seconds since server startup | `time(NULL) - startTime` |
| `<connection-fd>` | Accepted client socket descriptor | Return value of `accept()` |
| `<listener-fd>` | Listening server socket descriptor | `serverSocket` |
| `<client-ip>` | Client IP address | `inet_ntoa(clientAddr.sin_addr)` |
| `<client-port>` | Client TCP source port | `ntohs(clientAddr.sin_port)` |
| `<nick>` | Client nickname | `client.getNickname()` |
| `<user>` | Client username | `client.getUsername()` |
| `<host>` | Resolved host or client IP | `client.getIp()` if DNS is omitted |
| `<reason>` | Why the connection is closing | QUIT, EOF, socket error, etc. |

Suggested levels:

| Level | Name | Use |
|---:|---|---|
| `1` | alert | Fatal initialization failure |
| `3` | error | Failed system call or rejected client |
| `4` | warning | Recoverable configuration issue |
| `5` | notice | Major lifecycle event |
| `6` | info | Detailed connection or setup event |

The prefix is optional. Implement the message bodies first if a reusable logger
does not exist yet.

## 1. Server startup

### Process starts

```text
<server-name> <version> starting ...
```

ngIRCd-style example:

```text
ngIRCd 26.1 starting ...
```

Recommended level: `5`.

### Configuration or command-line settings loaded

If the server uses a configuration file:

```text
Using configuration file "<config-path>" ...
```

For this project, which currently uses command-line arguments:

```text
Using command-line configuration: port <port>.
```

Recommended level: `6`.

### I/O subsystem initialized

```text
IO subsystem: poll (initial descriptors <descriptor-count>).
```

Recommended level: `6`.

### Listening socket ready

```text
Now listening on [<listen-address>]:<port> (socket <listener-fd>).
```

Example:

```text
Now listening on [0.0.0.0]:6667 (socket 3).
```

Recommended level: `6`. Emit this only after `bind()` and `listen()` both
succeed.

### Server ready

```text
Server "<server-name>" ready.
```

Recommended level: `5`. This should be the final successful startup message,
immediately before entering the main `poll()` loop.

### Complete minimal startup sequence

```text
[<pid>:5    0] <server-name> <version> starting ...
[<pid>:6    0] Using command-line configuration: port <port>.
[<pid>:6    0] IO subsystem: poll (initial descriptors 1).
[<pid>:6    0] Now listening on [0.0.0.0]:<port> (socket <listener-fd>).
[<pid>:5    0] Server "<server-name>" ready.
```

## 2. New TCP connection

### Connection accepted

```text
Accepted connection <connection-fd> from "<client-ip>:<client-port>" on socket <listener-fd>.
```

Example:

```text
Accepted connection 7 from "127.0.0.1:51020" on socket 3.
```

Recommended level: `6`. Emit this immediately after `accept()` succeeds.

This project already captures the IP in `Server::acceptClient()`. Preserve the
source port before `clientAddr` leaves scope if it should appear in later
messages. Convert it with `ntohs(clientAddr.sin_port)`.

### Optional IDENT result

Only use these if IDENT lookup support is actually implemented:

```text
IDENT lookup for connection <connection-fd>: "<identity>".
IDENT lookup for connection <connection-fd>: no result.
```

Recommended level: `6`. Do not mimic this line if the server performs no IDENT
lookup.

## 3. IRC registration

### Client successfully registered

```text
User "<nick>!<user>@<host>" registered (connection <connection-fd>).
```

Example using the project's IP address as the host:

```text
User "alice!alice@127.0.0.1" registered (connection 7).
```

Recommended level: `5`. Emit this only when `Client::tryMarkRegistered()` changes
the registration state from false to true. The natural insertion point is
`CommandHandler::tryRegistration()` after that call succeeds.

### Client registration rejected

General template:

```text
User "<best-known-client-mask>" rejected (connection <connection-fd>): <reason>!
```

Useful concrete forms:

```text
User "<nick>!<user>@<host>" rejected (connection <connection-fd>): Bad password!
Client rejected (connection <connection-fd>): Registration timed out!
Client rejected (connection <connection-fd>): Invalid registration data!
```

Recommended level: `3`.

## 4. Connection shutdown begins

Use one consistent message before removing the client from maps, channels, and
the `pollfd` vector:

```text
Shutting down connection <connection-fd> (<reason>) with "<host>:<client-port>" ...
```

Recommended level: `6`.

Canonical reason strings:

| Event | `<reason>` |
|---|---|
| Client sent `QUIT` | `Got QUIT command` |
| `recv()` returned `0` | `Client closed connection` |
| `POLLHUP` | `Client closed connection` |
| `POLLERR` | `Socket error` |
| `POLLNVAL` | `Invalid socket descriptor` |
| Read failure | `Read error: <strerror(errno)>` |
| Write failure | `Write error: <strerror(errno)>` |
| Input exceeded the configured limit | `Input line too long` |
| Bad server password | `Bad password` |
| Server is stopping | `Server shutdown` |

## 5. Client unregistered

### Registered user disconnects

```text
User "<nick>!<user>@<host>" unregistered (connection <connection-fd>): <reason>.
```

Examples:

```text
User "alice!alice@127.0.0.1" unregistered (connection 7): Got QUIT command.
User "bob!bob@127.0.0.1" unregistered (connection 8): Client closed connection.
```

Recommended level: `5`.

### Unregistered TCP client disconnects

```text
Client unregistered (connection <connection-fd>): <reason>.
```

Example:

```text
Client unregistered (connection 7): Client closed connection.
```

Recommended level: `5`. Choose this form when `client.isRegistered()` is false.

## 6. Connection fully closed

With traffic counters:

```text
Connection <connection-fd> with "<host>:<client-port>" closed (in: <received-kb>k, out: <sent-kb>k).
```

Without traffic counters:

```text
Connection <connection-fd> with "<host>:<client-port>" closed.
```

Recommended level: `6`. Emit this after `close(connectionFd)` and cleanup have
completed. Start with the counter-free form unless byte counters are added to
`Client`.

### Complete graceful `QUIT` sequence

```text
[<pid>:6 <elapsed>] Accepted connection <connection-fd> from "<client-ip>:<client-port>" on socket <listener-fd>.
[<pid>:5 <elapsed>] User "<nick>!<user>@<host>" registered (connection <connection-fd>).
[<pid>:6 <elapsed>] Shutting down connection <connection-fd> (Got QUIT command) with "<host>:<client-port>" ...
[<pid>:5 <elapsed>] User "<nick>!<user>@<host>" unregistered (connection <connection-fd>): Got QUIT command.
[<pid>:6 <elapsed>] Connection <connection-fd> with "<host>:<client-port>" closed.
```

### Complete abrupt-disconnect sequence

```text
[<pid>:6 <elapsed>] Accepted connection <connection-fd> from "<client-ip>:<client-port>" on socket <listener-fd>.
[<pid>:5 <elapsed>] User "<nick>!<user>@<host>" registered (connection <connection-fd>).
[<pid>:6 <elapsed>] Shutting down connection <connection-fd> (Client closed connection) with "<host>:<client-port>" ...
[<pid>:5 <elapsed>] User "<nick>!<user>@<host>" unregistered (connection <connection-fd>): Client closed connection.
[<pid>:6 <elapsed>] Connection <connection-fd> with "<host>:<client-port>" closed.
```

## 7. Server shutdown

### Signal received

```text
Got signal "<signal-name>" ...
```

Examples:

```text
Got signal "Interrupt" ...
Got signal "Quit" ...
```

Recommended level: `6`.

### Shutdown sequence

```text
Server going down NOW!
Shutting down all listening sockets (<listener-count> total) ...
ngIRCd done, served <accepted-connection-count> connections.
```

Use the actual project/server name instead of `ngIRCd` in the last line:

```text
ft_irc done, served <accepted-connection-count> connections.
```

Recommended levels:

- `Server going down NOW!`: `5`
- Listening-socket cleanup: `6`
- Final summary: `5`

## 8. Startup and runtime errors

### Socket setup failures

```text
Can't create server socket: <system-error>!
Can't set server socket to non-blocking mode: <system-error>!
Can't bind [<listen-address>]:<port>: <system-error>!
Can't listen on [<listen-address>]:<port>: <system-error>!
```

Recommended level: `3`.

### Accept or client setup failures

```text
Can't accept a new connection on socket <listener-fd>: <system-error>!
Can't set connection <connection-fd> to non-blocking mode: <system-error>!
Can't track connection <connection-fd>: <error>!
```

Recommended level: `3`.

### Fatal initialization failure

```text
Fatal: Initialization failed, exiting!
```

Recommended level: `1`.

## Implementation map for the current project

| Event | Current location | Suggested action |
|---|---|---|
| Startup begins | `Server::serverInit()` | Replace `Connection succesfull` with the startup sequence |
| Listening begins | `Server::sockIt()` after `listen()` | Log address, port, and `serverSocket` |
| TCP accept | `Server::acceptClient()` after `accept()` | Log fd, IP, source port, and listener fd |
| IRC registration | `CommandHandler::tryRegistration()` | Log only after `tryMarkRegistered()` succeeds |
| `QUIT` | `CommandHandler::quit()` | Preserve `Got QUIT command` as the disconnect reason |
| EOF/read failure | `Server::retrieveData()` | Select the appropriate disconnect reason |
| Poll error/hangup | `Server::serverThread()` | Select the appropriate disconnect reason |
| Client cleanup | `Server::disconnectClient()` | Log shutdown, unregistration, then final closure |
| Signal shutdown | `Server::serverThread()` exit path | Log signal/shutdown summary |

The current `disconnectClient(int fd)` interface does not receive a reason. To
produce accurate messages, eventually change it to something like:

```cpp
void disconnectClient(int fd, const std::string &reason);
```

Preserve all client fields needed for logging **before** erasing the client from
`clients`. If the source port and traffic counters are desired, add them to
`Client`; otherwise use the simpler templates that omit those values.

## Minimal first implementation

If implementing everything at once is unnecessary, begin with these five lines:

```text
Now listening on [0.0.0.0]:<port> (socket <listener-fd>).
Accepted connection <connection-fd> from "<client-ip>:<client-port>" on socket <listener-fd>.
User "<nick>!<user>@<host>" registered (connection <connection-fd>).
User "<nick>!<user>@<host>" unregistered (connection <connection-fd>): <reason>.
Connection <connection-fd> with "<host>:<client-port>" closed.
```

These cover the most useful lifecycle events while keeping the first code change
small.
