# rjkshelltools - Various commands

## Installation

    autoreconf -si
    ./configure
    make
    sudo make install

# Tools

This package contains various commands that might be useful to shell
users.
Each program has its own man page.

## inplace

This tool allows in-place modification of multiple files, via any
filter.  See `README.inplace`.

## adverbio

This tool provides I/O redirection operations without needing to
invoke the shell (thus removing the need to perform shell quoting
on complex commands).  See `README.adverbio`.

## alarm

This tool executes a command and (if it doesn't terminate first)
sends it a signal after a certain amount of time.  The idea is to
time out commands that get "stuck", or just take too long.

## logfds

This tool executes a command as a subprocess, and logs the output
it writes to selected file descriptors to logfiles.  It can handle
compression and expiry of old logfiles automatically.

## bind-socket

This tool opens one or more sockets (using chosen file
descriptors), binds them to specified addresses and configures
them to listen for incoming connections.

## accept-socket

This tool accepts connections on an already listening socket
(e.g. one opened by bind-socket) and executes a command in a
subprocess to handle them.

## connect-socket

This tool connects a socket using a chosen file descriptor, and
then executes a command.

## pidfile

A very simple script to drop a pidfile and then continue by
executing a further command.

## run-as

This tool runs a command with a chosen user ID (and group ID).

## daemon

This is a tool for daemonizing commands.

## run-repeatedly

Runs programs found in a directory, restarting them when they
terminate.

## with-lock

Runs a program whild holding an fcntl-style lock.

## iobuffer

Copies input to output, guaranteeing minimum read and write sizes

# Concepts

## Setting Up Daemons

The point of `logfds`, `bind-socket`, et al, is to provide flexible ways
to set up daemons and so on.  For example:

    daemon -- \
          logfds -c -m 14 1,2 -- /var/log/myserver/%Y/%m/%d.log \
          bind-socket -- 0 inet stream 90 \
          pidfile -- /var/run/myserver.pid \
          run-as -- myuser -- \
          myserver

This does the following:

* runs in background
* logs stderr and stdout output to log files under
`/var/log/myserver`, automatically compressing log files other than
the current one and deleting logs older than a fortnight
* binds standard input to TCP port 90
* drops a pidfile in `/var/run`
* switches to an unprivileged user
* runs the `myserver` program

Pidfiles are a poor way of keeping track of daemons, however; it is
possible for them to be left behind even after the daemon
terminates.  (Even if the daemon contains code to clean it up, it
may receive a `SIGKILL` or a `SIGSEGV` that it can't handle safely.)
Once the daemon has terminated the PID may be re-used, and if
something reads the pidfile when this has happened the result may be
e.g. that a signal is sent to the wrong process.

For the case where you want to run multiple daemons, run-repeatedly
can be run as a daemon and execute the others via shell scripts in
its control directory.  To achieve a similar effect to the above,
one of these scripts might look like this:

    #! /bin/sh
    set -e
    exec logfds -C -c -m 14 1,2 -- /var/log/myserver/%Y/%m/%d.log \
         bind-socket -- 0 inet stream 90 \
         run-as -- myuser -- \
         myserver

Note the use of exec and the -C option to ensure that run-repeatedly
signals the appropriate PID.  run-repeatedly does not have the above
problem with pidfiles, as it is notified by the operating system
when its children terminate.  (Of course if run-repeatedly crashes
you have an even worse problem, but it is a small and simple
program, and there - hopefully! - less likely to crash than large
and complex servers.)

## Sockets

`accept-socket` and `connect-socket` were mainly written to test
`bind-socket`, though they have potential for wider use.  For example,
to write as server which handles requests on a UNIX-domain socket
much as if it were invoked from inetd, you might use a fragment like
the following:

    bind-socket -- 3 unix stream /path/to/socket -- \
	accept-socket -- 3 0 -- \
	adverbio --redirect 1:0 -- \
	/path/to/server/program

# Feedback

Please report bugs via github.

# Copyright

Copyright © 2001, 2002, 2014, 2015 Richard Kettlewell

rjkshelltools is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
