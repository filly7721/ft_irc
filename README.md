*This project has been created as part of the 42 curriculum by filly7721.*

# ft_irc — Internet Relay Chat Server

## Description

ft_irc is a C++98 IRC server built from scratch as part of the 42 school curriculum. IRC (Internet Relay Chat) is a text-based protocol for real-time communication over the Internet, supporting both public group channels and private direct messages.

The server handles multiple simultaneous client connections through a single non-blocking `poll()` event loop — no threads, no forking. Clients connect using any standard IRC client, authenticate with a password, and can join channels, exchange messages, and manage channel settings through a full set of IRC commands.

The project is structured around three core classes: `Server` (network and client lifecycle management), `Client` (per-connection state and command handling), and `Channel` (group membership and modes).

## Architecture

```
ft_irc/
├── main.cpp        # Entry point, signal handling, IRC message parser
├── Server.cpp/hpp  # Socket setup, poll() loop, client/channel management
├── Client.cpp/hpp  # Per-client state, buffer handling, all IRC commands
├── Channel.cpp/hpp # Channel data: members, operators, invites, modes
└── ft_irc.h        # Shared types: Command struct, parseMessage(), g_server
```

**Event loop overview:**
1. `poll()` monitors all file descriptors (server socket + all client sockets)
2. New connections are accepted and added to the poll set
3. Incoming data is appended to a per-client buffer
4. After all I/O is processed, each client's buffer is parsed line-by-line into `Command` structs and dispatched to the appropriate handler
5. Disconnected clients are cleaned up at the end of each loop iteration
6. On a 30-second timeout with no activity, the server pings all clients and drops any that don't respond

## Features

**Authentication & Registration:**
The registration flow requires `PASS` → `NICK` + `USER` in that order. The server only marks a client as registered once all three conditions are met: password verified, nickname set, and username/realname provided.

**Commands implemented:**

| Command | Access | Description |
|---------|--------|-------------|
| `PASS`    | Public | Set connection password |
| `NICK`    | Public | Set or change nickname (max 9 chars, alphanumeric + `_` `-`) |
| `USER`    | Public | Set username and realname |
| `PONG`    | Public | Reply to server PING (keepalive) |
| `QUIT`    | Public | Disconnect from the server |
| `PRIVMSG` | Registered | Send a message to a user or channel |
| `JOIN`    | Registered | Join one or more channels (comma-separated) |
| `PART`    | Registered | Leave one or more channels |
| `KICK`    | Operator | Eject a user from a channel |
| `INVITE`  | Operator | Invite a user to a channel |
| `TOPIC`   | Operator/Member | View or set the channel topic |
| `MODE`    | Operator | View or change channel modes |

**Channel modes (MODE):**

| Flag | Description |
|------|-------------|
| `+i` / `-i` | Set / remove invite-only |
| `+t` / `-t` | Restrict TOPIC changes to operators only |
| `+k` / `-k` | Set / remove channel key (password) |
| `+o` / `-o` | Grant / revoke operator privilege |
| `+l` / `-l` | Set / remove user limit |

**Other behaviours:**
- Channels are created automatically on the first `JOIN` and destroyed when empty
- Channel names are stored and looked up case-insensitively
- All IRC messages are CRLF-terminated and partial packets are buffered correctly
- The server responds to SIGINT, SIGTERM, and SIGQUIT with a graceful shutdown

## Instructions

### Requirements

- A Unix-like system (Linux or macOS)
- A C++ compiler supporting the **C++98** standard
- An IRC client — the reference client used for this project is **irssi**

### Compilation

```bash
make
```

Other Makefile targets: `clean`, `fclean`, `re`.

The binary is compiled with `c++ -std=c++98 -Wall -Wextra -Werror`.

### Running the server

```bash
./ft_irc <port> <password>
```

Example:
```bash
./ft_irc 6667 secret
```

The server will start listening on the given port. Stop it cleanly with `Ctrl+C`.

### Connecting with irssi

```
/connect -nocap localhost <port> <password>
```

### Connecting with netcat (for testing)

```bash
nc -C localhost <port>
```

Then manually send IRC commands:
```
PASS secret
NICK mynick
USER myuser 0 * :My Real Name
JOIN #test
PRIVMSG #test :hello
```

### Testing partial data handling

The server aggregates fragmented packets before processing. To test this with netcat, use `Ctrl+D` to flush data mid-message:

```bash
nc -C 127.0.0.1 6667
# Type: PAS^D  S sec^D  ret^D  <Enter>
# The server buffers all parts and only processes the command once \n arrives
```

## Resources

### IRC Protocol

- [RFC 2812 — IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [RFC 2811 — IRC Channel Management](https://datatracker.ietf.org/doc/html/rfc2811)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) — sockets, `poll()`, non-blocking I/O

### AI Usage

Claude was used during this project for the following tasks:

- Clarifying IRC protocol message formatting (prefix parsing, CRLF termination, numeric reply format)
- Writing the boiler plate code for all the command handling.
- CPP string methods consultation.
- Generating this README

All AI-generated content was reviewed, understood, and tested before use. No code was copied without being manually read and verified. Peer review was used throughout as a quality check.