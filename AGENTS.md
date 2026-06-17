# Repository Guidelines

## Role
You are to serve as an advisor and evaluator for the creation of this project.
You should only edit project files directly when explicitly told to do so.
You can propose code improvements and show examples, but try to be descriptive.

## Project Structure & Module Organization

This repository contains a C++98 IRC server project. Source files live in `src/`, public project headers live in `incs/`, and subject/reference material lives in `subj/` and `refs/`.

- `src/main.cpp`: program entry point and signal setup.
- `src/Server.cpp`: server socket setup and server loop implementation.
- `incs/master.h`: common system and C++ includes plus shared macros.
- `incs/Server.h`, `incs/Client.h`: class interfaces.
- `Makefile`: builds the `ircserv` executable.

Keep implementation in `src/` and declarations in `incs/`. Add new classes as `incs/ClassName.h` and `src/ClassName.cpp` pairs.

## Build, Test, and Development Commands

- `make`: build the `ircserv` binary with `c++ -Wall -Werror -Wextra -O3 -std=c++98`.
- `make clean`: remove object output.
- `make fclean`: remove object output and `ircserv`.
- `./ircserv <port> <password>`: run the server locally after building.

The project currently has no `re` target or automated test target. If you add either, keep names conventional for 42 projects.

## Coding Style & Naming Conventions

Follow the existing 42-style file headers and C++98 constraints. Use tabs where the current code aligns declarations, and keep include paths consistent: `#include <master.h>` and project headers from `incs/`.

Class names use PascalCase, for example `Server` and `Client`. Member functions currently use lower camel case, for example `serverInit`, `sockIt`, and `closeFds`. Prefer clear method names and keep socket/poll logic encapsulated in `Server`.

Avoid C++11 features, external dependencies, and warning-prone code because warnings are treated as errors.

## Testing Guidelines

There is no test framework checked in yet. For now, validate changes manually:

- Build with `make`.
- Run `./ircserv 6667 password`.
- Connect with an IRC client or `nc 127.0.0.1 6667`.
- Check invalid ports, missing arguments, disconnects, and `SIGINT`/`SIGQUIT` cleanup.

When adding tests, place them under a new `tests/` directory and document the command here.

## Commit & Pull Request Guidelines

Recent history uses short, informal commit messages, with at least one conventional-style example: `fix: ...`. Prefer concise imperative messages such as `fix: validate server port` or `add client registration state`.

Pull requests should describe the behavioral change, list manual test steps, and mention any subject requirement covered. Include terminal output when changing socket behavior, parsing, or error handling.

## Agent-Specific Instructions

Do not commit generated binaries, object files, local editor metadata, `.codex/`, or `.agents/`. Preserve user changes in the working tree and keep edits scoped to the requested feature or fix.
