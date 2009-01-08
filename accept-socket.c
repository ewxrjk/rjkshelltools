/* 
   accept-socket - accept connections on a socket and run a command
   for each

   Copyright (C) 2001, 2002 Richard Kettlewell

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

/* XXX add some sort of access control for INET sockets; maybe also
 * for UNIX sockets on platforms that can pass credentials across
 * sockets. */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>

#include "utils.h"

/* Option flags and variables */
static struct option const long_options[] =
{
  { "verbose", no_argument, 0, 'v' },
  { "numeric", no_argument, 0, 'n' },
  { "no-close", no_argument, 0, 'c' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  accept-socket [options] [--] lfd cfd [--] command ...\n"
"\n"
"Options:\n"
"  -v, --verbose                 Verbose mode\n"
"  -c, --no-close                Don't close listening socket\n"
"  -n, --numeric                 Don't resolve hostnames\n"
"  -h, --help                    Usage message\n"
"  -V, --version                 Version number\n"
"\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

static volatile sig_atomic_t sigchld;

static void sigchld_handler(int __attribute__((unused)) sig) {
  sigchld = 1;
}

/* collect any zombie processes, if verbose != 0, report their PID and
 * exit status */

static void collect_zombies(int verbose) {
  pid_t pid;
  int status;
  
  while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if(verbose)
      error("[%lu] %s", (unsigned long)pid, wstat(status));
  }
}

int main(int argc, char **argv) {
  int n;
  union {
    struct sockaddr sa;
    struct sockaddr_in in;
    struct sockaddr_un un;
  } u;
  int lfd, cfd;
  int closeit = 1;
  int verbose = 0;
  int rdns = 1;
  sigset_t chmask;
  struct sigaction sa;
  
  setprogname(argv[0]);
  
  while((n = getopt_long(argc, argv, 
			 "hVnvc",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("accept-socket %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);

    case 'c':
      closeit = 0;
      break;

    case 'v':
      verbose++;
      break;

    case 'n':
      rdns = 0;
      break;
      
    default:
      usage(stderr, 1);
    }
  }

  /* parse arguments */
  if(optind >= argc || !isdigit((unsigned char)argv[optind][0]))
    fatal("no file listening descriptor specified");
  lfd = atoi(argv[optind++]);

  if(optind >= argc || !isdigit((unsigned char)argv[optind][0]))
    fatal("no file connected descriptor specified");
  cfd = atoi(argv[optind++]);

  /* skip -- if present */
  if(optind < argc && !strcmp(argv[optind], "--"))
    ++optind;
  
  if(optind >= argc)
    fatal("no command specified");

  /* make listening FD nonblocking */
  nonblock(lfd);

  sigemptyset(&chmask);
  sigaddset(&chmask, SIGCHLD);

  sa.sa_handler = sigchld_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction_e(SIGCHLD, &sa, 0);
  
  for(;;) {
    socklen_t len = sizeof u;
    int fd;
    fd_set fds;
    struct timeval tv;
    int p[2];
    char buffer[1];

    sigprocmask_e(SIG_UNBLOCK, &chmask, 0);
    
    FD_ZERO(&fds);
    FD_SET(lfd, &fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    n = select(lfd + 1, &fds, 0, 0, &tv);
    if(n < 0 && errno != EINTR)
      fatale("error calling select");

    sigprocmask_e(SIG_BLOCK, &chmask, 0);

    if(sigchld) {
      collect_zombies(verbose);
      sigchld = 0;
    }
    
    if(n == 0)
      continue;
    fd = accept(lfd, &u.sa, &len);
    if(fd < 0) {
	if(verbose && errno != EINTR && errno != EAGAIN)
	errore("error calling accept");
      continue;
    }
    /* we use a pipe to detect when the child exits or executes.  This
     * is so that messages to stderr from the child are unlikely to
     * get interleaved with messages from the parent. */
    pipe_e(p);
    switch(fork()) {
    case -1:
      errore("error calling fork");
      break;
    default:
      break;
    case 0:
      exiter = _exit;
      /* if the writer end of the pipe clashes with CFD, dup it out of
       * the way.  The extra copy of the writer end that gets left
       * behind will not be made FD_CLOEXEC, but instead be smashed
       * when CFD is dup'd into place. */
      if(p[1] == cfd)
	p[1] = dup_e(cfd);
      cloexec(p[1]);
      close_e(p[0]);
      /* in verbose mode, report the origin of the connection */
      if(verbose)
	error("[%lu] %s",
	      (unsigned long)getpid(), socketprint(&u.sa, len, rdns));
      if(closeit)
	close_e(lfd);
      if(fd != cfd) {
	dup2_e(fd, cfd);
	close_e(fd);
      }
      if(execvp(argv[optind], argv + optind) < 0)
	fatale("error executing %s", argv[optind]);
      fatal("execvp %s unexpectedly returned", argv[optind]);
    }
    close_e(p[1]);
    close_e(fd);
    /* wait for child to exec */
    read(p[0], buffer, 1);
    close_e(p[0]);
    /* do a speculative wait.  If the child has terminated by now
     * (e.g. if it encountered an error setting up) then without this
     * we only reap it after select times out, as the signal is
     * delivered when we change the signal mask, and therefore does
     * not interrupt select. */
    collect_zombies(verbose);
  }
}


/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
