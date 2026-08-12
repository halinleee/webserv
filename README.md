*This project has been created as part of the 42 curriculum by halee, gajeon, seungsch.*

# webserv

*This project has been created as part of the 42 curriculum by halee, gajeon, seungsch.*

## Description

`webserv` is an HTTP/1.1 server implemented in C++98. The project's goal was to implement core parts of an HTTP server — request parsing, routing, static file serving, CGI — similar to what NGINX does.

The server parses HTTP requests, routes them to the appropriate server, serves static files, handles uploads and deletions, runs CGI scripts, and performs all I/O without blocking.

Internally, a single `epoll` loop (`src/server/Epoll.cpp`) monitors all listening sockets and client connections — no threads and no blocking `read`/`write`. Servers, error pages, size limits, timeouts, and routing rules are all defined in a single NGINX-style configuration file.

## Instructions

### Compilation

```sh
make        # builds the "webserv" executable
make clean  # removes object files
make fclean # removes object files and the executable
make re     # fclean + all
```

The project is compiled with `c++` using `-Wall -Wextra -Werror -std=c++98`.

### Running

```sh
./webserv [path/to/config_file]
```

If no configuration file is given, `./webserv.conf` at the repository root is used by default.

An example configuration covering all features (static content, uploads, autoindex, redirection, and CGI) is provided in `webserv.conf`. It serves content from `www/`.

Example:

```sh
./webserv webserv.conf
```

Then visit `http://localhost:8080` (main site) and `http://localhost:8081` (admin site) in a browser.

### Configuration file format

The configuration is inspired by NGINX's `server` block syntax. Each `server` block defines one server:

```text
server <port>

    client_max_body_size <bytes>
    error_page <code> <path>
    max_client <n>
    listen <interface>

    connection_timeout <seconds>
    read_timeout <seconds>
    write_timeout <seconds>
    keep_alive_timeout <seconds>
    cgi_timeout <seconds>

    location <path>
        root <directory>
        alias <directory>
        index <file>
        methods <METHOD ...>
        autoindex on|off
        return <code> <location>
        cgi_ext <extension> <interpreter_path>
end
```

See `webserv.conf` for a complete working example.

## Resources

* [RFC 9110 — HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110)
* [RFC 9112 — HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112)
* [RFC 7230 — Message Syntax and Routing](https://www.rfc-editor.org/rfc/rfc7230)
* [The Common Gateway Interface (CGI) Specification, RFC 3875](https://www.rfc-editor.org/rfc/rfc3875)
* [NGINX documentation](https://nginx.org/en/docs/) — used as a reference for configuration syntax and for comparing server behaviour.
* `man` pages for `epoll`, `socket`, `fork`, `execve`, and `poll`.

### AI usage

AI assistants (Claude) were used during this project in a supporting role: explaining sections of the HTTP RFCs in plain language, discussing trade-offs in the non-blocking `epoll`-based event loop design, helping draft and debug test scripts, and reviewing.
