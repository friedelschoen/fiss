# fiss (Friedel's Initialization and Service Supervision)

> 0.3.3 (May 2023)

fiss is a lightweight and easy-to-use tool for initializing and supervising long-running services on Unix-like systems. It provides a simple and reliable way to start, stop, and monitor services. It supports automatic restarts, logging, and customizable startup options.

fiss is inspired by tools like [runit](http://smarden.org/runit/) and [daemontools](http://cr.yp.to/daemontools.html), but it aims to be simpler and more flexible, while still providing a similar level of reliability and security.

## Features

fiss provides the following features:

-   Automatic restarts: If a service exits unexpectedly, fiss will automatically restart it, up to a configurable number of times.
-   Logging: fiss captures the stdout and stderr of the service and writes them to log files, which can be rotated and compressed.
-   Customizable startup options: fiss allows you to specify environment variables, working directory, umask, and other options for the service.
-   Supervision: fiss monitors the service and ensures that it stays running, or else it terminates the service and retries later.
-   Status monitoring: fiss provides a simple command-line interface to check the status of a service, including its uptime, PID, and exit code.
-   Simple configuration: fiss uses a simple directory structure to store the configuration for each service, making it easy to manage and version control.


## Thanks to

Thanks to D. J. Bernstein for [`daemontools`](http://cr.yp.to/daemontools.html) and build a sane and beautiful service management system. 

Thanks to G. Pape to contribute and extend `daemontool` by building the init-system [`runit`](http://smarden.org/runit/).

Thanks to the contributers of [Void Linux](`https://voidlinux.org/`) for the [`runit-tools`](https://github.com/void-linux/void-runit) to make some scripts and programs around `runit` to make it an complete init-system for linux. 

Without you, `fiss` would not be possible :heart:!

## Contributing

Contributing is always welcome :grin:! 

This is possible by issueing problems or ideas at [GitHub](https://github.com/friedelschoen/fiss/), I'll look forward to implement them.

Or if you got some free time left, you can add code to this project do a pull-request! To help you on your way, a little description:

| directory             | description                                           |
|-----------------------|-------------------------------------------------------|
| .github/              | workflows for github, like building                   |
| assets/               | assets for the docs-website                           |
| bin/                  | resulting executables                                 |
| bin/*.c               | C-sources                                             |
| bin/*.lnk             | links to other executables<sup>1</sup>                |
| bin/*.sh              | shell-scripts                                         |
| contrib/              | helpful files for contributing                        |
| contrib/command.txt   | internal commands for service's `supervise/control`   |
| contrib/docs.txt      | custom documentation and manual format in docs/       |
| contrib/make.txt      | custom make-headers                                   |
| contrib/serialize.txt | serialization of `supervise/status`                   |
| docs/                 | pages to include on the website                       |
| include/              | C-headers                                             |
| man/                  | manuals                                               |
| mk/                   | common files for `make`                               |
| share/                | files for `fiss`, should be installed at `/usr/share` |
| src/                  | C-sources for executables                             |
| tools/                | tools for building                                    |
| tools/make-docs.py    | converts a `.txt`-file to `.html`                     |
| tools/make-man.py     | converts a `.txt`-file to a roff-file                 |
| .clang-format         | format-specification for `clang-format`               |
| configure             | configure behaviour of the resulting executables      |
| fiss.svg              | logo of fiss                                          |
| LICENSE               | license-text (zlib-license)                           |
| Makefile              | makefile for gnu make                                 |
| readme.md             | what you are reading right now                        | 

- <sup>1</sup>plain text files containing the name of the symbolic link

> :warning: please format your modification using `clang-format` and the `.clang-format` specifications!

## Licensing

This project is licensed under the terms of the [zlib license](./LICENSE).
