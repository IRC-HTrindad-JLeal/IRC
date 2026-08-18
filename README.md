*This project has been created as part of the 42 curriculum by [mely-pan], [jleal], [htrindad].*
 
# ft_irc
 
## Description
 
ft_irc is a fully functional IRC server written in C++98, built as part of the 42 school curriculum. The goal of the project is to implement the core of the IRC protocol from scratch, handling multiple simultaneous client connections, authentication, channel management, and a standard set of IRC commands, without using any external libraries or multi-threading.
 
The server follows the IRC protocol as defined in RFC 1459 and RFC 2812, and is compatible with standard IRC clients such as irssi.
 
## Instructions
 
### Requirements
 
- A C++98-compatible compiler (g++ or clang++)
- GNU Make
- A Unix-based system (Linux or macOS)

### Compilation
 
```bash
make
```
 
### Running the server
 
```bash
./ircserv <port> <password>
```
 
- `<port>` — the port number the server will listen on (e.g. `6667`)
- `<password>` — the connection password clients must provide

### Connecting with a client
 
Using netcat for quick testing:
```bash
nc -C 127.0.0.1 6667
PASS <password>
NICK mynick
USER mynick 0 * :My Name
```
 
### Cleanup
 
```bash
make clean	# removes object files
make fclean	# removes object files and binary
make re		# recompilation
```
 
## Features
 
### Connection & Authentication
- `PASS` — server password authentication
- `NICK` — set or change nickname (with validity and uniqueness checks)
- `USER` — register username and realname
- `CAP` — minimal IRCv3 capability negotiation
- `PING` / `PONG` — keepalive handling
- `QUIT` — graceful client disconnection with channel broadcast

### Channel Operations
- `JOIN` — join or create channels, with support for channel keys and invite lists
- `PRIVMSG` — send messages to channels or directly to users
- `TOPIC` — view or set the channel topic (respects `+t` mode)
- `INVITE` — invite users to channels (respects `+i` mode)
- `KICK` — remove users from channels (operators only)

### Channel Modes
- `+i` — invite-only
- `+t` — topic restricted to operators
- `+k` — channel key (password)
- `+o` — grant/revoke operator status
- `+l` — user limit

## Usage Examples
 
### Register and join a channel
```
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :Hello everyone!
```
 
### Set channel modes
```
MODE #general +k mykey		# set a channel password
MODE #general +l 10			# limit to 10 users
MODE #general +o bob		# give bob operator status
MODE #general +i-t			# invite-only, anyone can change topic
```
 
### Kick a user
```
KICK #general bob :spamming
```
 
### Invite a user to a private channel
```
MODE #secret +i
INVITE carol #secret
```
 
## Resources
 
### IRC Protocol
- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Modern IRC Documentation](https://modern.ircdocs.horse)
- [Bircd manual](https://ircd.bircd.org/manual.html)

### Tools
- [irssi IRC client](https://irssi.org)

### AI Usage
 
Claude and Codex was used throughout the development of this project as a technical assistant. Its use was limited to the following tasks:
 
- **Code Review** — helping review code implementation and give feedback
- **Research** — helping with research and general explanaitons on the subject of IRC servers and the IRC protocol
- **Debugging** — Creating debugging programs and test harnesses
- **Documentation** — helping design this readme file and other reference files
All code was written, understood, and validated by the project authors. AI was used as a reference and review tool, not as a code generator.
 