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

## Licensing

This project is licensed under the terms of the [zlib license](./LICENSE).
