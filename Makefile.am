# 
# This file is part of rjkshelltools
# Copyright (C) 2001, 2002, 2013 Richard Kettlewell
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
# 

noinst_LIBRARIES=libutils.a

bin_SCRIPTS=pidfile
bin_PROGRAMS=adverbio inplace alarm daemon logfds bind-socket run-as \
	run-repeatedly connect-socket accept-socket with-lock iobuffer \
	anagrams

adverbio_SOURCES=adverbio.c

inplace_SOURCES=inplace.c

alarm_SOURCES=alarm.c

daemon_SOURCES=daemon.c utils.h logdaemon.h

logfds_SOURCES=logfds.c

bind_socket_SOURCES=bind-socket.c

connect_socket_SOURCES=connect-socket.c

accept_socket_SOURCES=accept-socket.c

run_as_SOURCES=run-as.c

run_repeatedly_SOURCES=run-repeatedly.c

with_lock_SOURCES=with-lock.c

iobuffer_SOURCES=iobuffer.c uio.c uio.h

anagrams_SOURCES=anagrams.c

LDADD=libutils.a

libutils_a_SOURCES=mem.c fatal.c maxfd.c cloexec.c nonblock.c write.c \
xstrdupcat.c error.c logdaemon.c fdmap.c split.c wstat.c strsignal.c \
xmemdup.c lookup.c inetaddress.c makedirs.c dirname.c setpriv.c progname.c \
xstrdupcat3.c lookupi.c signals.c sigloop.c socketarg.c socketprint.c \
getline.c hash.c open.c close.c dup2.c pipe.c sigaction.c sigprocmask.c \
fork.c fcntl.c waitpid.c dup.c setsid.c debug.c \
logdaemon.h utils.h

man_MANS=adverbio.1 inplace.1 alarm.1 daemon.1 logfds.1 bind-socket.1 \
	pidfile.1 connect-socket.1 run-as.1 accept-socket.1 with-lock.1 \
	iobuffer.1 anagrams.1

EXTRA_DIST=$(man_MANS) getopt.c getopt1.c getopt.h \
	README.inplace README.adverbio pidfile.in \
	tests.sh $(TESTS)
	getopt.c getopt.h getopt1.c

TESTS=test-run-repeatedly test-inplace test-logfds test-daemon \
	test-pidfile test-bind-socket test-with-lock test-anagrams
