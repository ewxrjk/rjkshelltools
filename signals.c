/*

   This file is part of rjkshellutils, Copyright (C) 2001 Richard Kettlewell

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <config.h>

#include <signal.h>
#include <sys/types.h>

#include "utils.h"

/* lookup table for signal names */

const struct lookuptable signallookup[] = {
#ifdef SIGHUP
  { "HUP", SIGHUP },
#endif
#ifdef SIGINT
  { "INT", SIGINT },
#endif
#ifdef SIGQUIT
  { "QUIT", SIGQUIT },
#endif
#ifdef SIGILL
  { "ILL", SIGILL },
#endif
#ifdef SIGTRAP
  { "TRAP", SIGTRAP },
#endif
#ifdef SIGABRT
  { "ABRT", SIGABRT },
#endif
#ifdef SIGIOT
  { "IOT", SIGIOT },
#endif
#ifdef SIGBUS
  { "BUS", SIGBUS },
#endif
#ifdef SIGFPE
  { "FPE", SIGFPE },
#endif
#ifdef SIGKILL
  { "KILL", SIGKILL },
#endif
#ifdef SIGUSR1
  { "USR1", SIGUSR1 },
#endif
#ifdef SIGSEGV
  { "SEGV", SIGSEGV },
#endif
#ifdef SIGUSR2
  { "USR2", SIGUSR2 },
#endif
#ifdef SIGPIPE
  { "PIPE", SIGPIPE },
#endif
#ifdef SIGALRM
  { "ALRM", SIGALRM },
#endif
#ifdef SIGTERM
  { "TERM", SIGTERM },
#endif
#ifdef SIGSTKFLT
  { "STKFLT", SIGSTKFLT },
#endif
#ifdef SIGCHLD
  { "CHLD", SIGCHLD },
#endif
#ifdef SIGCONT
  { "CONT", SIGCONT },
#endif
#ifdef SIGSTOP
  { "STOP", SIGSTOP },
#endif
#ifdef SIGTSTP
  { "TSTP", SIGTSTP },
#endif
#ifdef SIGTTIN
  { "TTIN", SIGTTIN },
#endif
#ifdef SIGTTOU
  { "TTOU", SIGTTOU },
#endif
#ifdef SIGURG
  { "URG", SIGURG },
#endif
#ifdef SIGXCPU
  { "XCPU", SIGXCPU },
#endif
#ifdef SIGXFSZ
  { "XFSZ", SIGXFSZ },
#endif
#ifdef SIGVTALRM
  { "VTALRM", SIGVTALRM },
#endif
#ifdef SIGPROF
  { "PROF", SIGPROF },
#endif
#ifdef SIGWINCH
  { "WINCH", SIGWINCH },
#endif
#ifdef SIGIO
  { "IO", SIGIO },
#endif
#ifdef SIGPOLL
  { "POLL", SIGPOLL },
#endif
#ifdef SIGLOST
  { "LOST", SIGLOST },
#endif
#ifdef SIGPWR
  { "PWR", SIGPWR },
#endif
#ifdef SIGSYS
  { "SYS", SIGSYS },
#endif
  { 0, 0 },
};

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
