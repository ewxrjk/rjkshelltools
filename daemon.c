/*
   daemon - run program as a daemon

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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "logdaemon.h"

static int closefds = 1;
static int changedir = 1;

/* Option flags and variables */

static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "no-close", no_argument, 0, 'n' },
  { "no-chdir", no_argument, 0, 'C' },
  { "log-stderr", required_argument, 0, 'l' },
  { "log-stdout", required_argument, 0, 'L' },
  { "compress", no_argument, 0, 'c' },
  { "max-log-age", required_argument, 0, 'm' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  daemon [options] [--] command ...\n"
"\n"
"Options:\n"
"  -C, --no-chdir                        Don't \"cd /\"\n"
"  -n, --no-close                        Don't close file descriptors\n"
"  -h, --help                            Usage message\n"
"  -l PATTERN, --log-stderr PATTERN      Log standard error to a file\n"
"  -L PATTERN, --log-stdout PATTERN      Log standard output to a file\n"
"  -c, --compress                        Compress logs\n"
"  -m DAYS, --max-log-age DAYS           Delete old logs\n"
"  -V, --version                         Version number\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

int main(int argc, char **argv) {
  int n;
  int nullfd;
  int pipefds[2];
  pid_t pid, r;
  int max = 0;
  int compress = 0;
  struct sigaction sa, osa;
  const char *logs[3] = { 0, 0, 0 };

  setprogname(argv[0]);

  while((n = getopt_long(argc, argv,
			 "hVncl:m:CL:",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("daemon %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);
      return 0;

    case 'n':
      closefds = 0;
      break;

    case 'C':
      changedir = 0;
      break;

    case 'm':
      max = atoi(optarg);
      break;

    case 'c':
      compress = 1;
      break;

    case 'l':
      logs[2] = optarg;
      break;

    case 'L':
      logs[1] = optarg;
      break;

    default:
      usage(stderr, 1);
    }
  }

  if(optind >= argc)
    fatal("no command specified");

  /* zap unwanted file descriptors (before we open log pipes!) */
  if(closefds) {
    for(n = maxfd(); n > 2; --n)
      close(n);
  }

  if(changedir) {
    /* change directory */
    if(chdir("/") < 0)
      fatale("error calling chdir");
  }

  if(logs[1] || logs[2]) {
    /* set up a logging daemon straight away */
    int p[3][2];
    struct logfile *l[3];
    int fd;

    /* create the pipes */
    for(n = 1; n <= 2; ++n)
      if(logs[n]) {
	pipe_e(p[n]);
	l[n] = ld_new_logfile(logs[n]);
	l[n]->rotate = max;
	l[n]->compress = compress;
	ld_new_input(p[n][0], l[n]);
      }
    /* create the daemon */
    if(!(pid = fork_e())) {
      exiter = _exit;
      setsid_e();
      for(n = 1; n <= 2; ++n)
	if(logs[n])
	  close_e(p[n][1]);
      fd = open_e("/dev/null", O_RDWR, 0);
      /* zap the real stdin/stdout/stderr */
      for(n = 0; n < 3; ++n)
	dup2_e(fd, n);
      if(fd >= 3)
	close_e(fd);
      if(!fork_e())
	ld_loop();
      _exit(0);
    }
    /* close the input ends now */
    for(n = 1; n <= 2; ++n)
      if(logs[n])
	close_e(p[n][0]);
    /* wait for the intermediate subprocess to terminate before
     * slamming stderr */
    r = waitpid_e(pid, &n, 0);
    if(n)
      fatal("logger subprocess %s", wstat(n));
    /* redirect stdout and/or stderr to the pipes */
    for(n = 1; n <= 2; ++n)
      if(logs[n] && p[n][1] != n) {
	dup2_e(p[n][1], n);
	close_e(p[n][1]);
      }
  }

  /* first fork... */
  if((pid = fork_e())) {
    /* wait for the intermediate subprocess (that will become the new
     * session leader) to terminate and notify the caller of any error
     * via our exit status */
    exiter = _exit;
    r = waitpid_e(pid, &n, 0);
    if(n)
      fatal("subprocess %s", wstat(n));
    _exit(0);
  }

  /* Stevens says the child will get a SIGHUP when the session leader
   * terminates, so we pre-emptively ignore it.  We'll restore it when
   * we know that the session leader has gone. */
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction_e(SIGHUP, &sa, &osa);

  /* in the first child */

  /* create a pipe so that the ultimate child knows when we
   * terminate */
  pipe_e(pipefds);

  /* zap stdin/stdout/stderr */
  if(closefds) {
    nullfd = open_e("/dev/null", O_RDWR, 0);
    for(n = 0; n <= 2; ++n)
      if(!logs[n] && nullfd != n)
	dup2_e(nullfd, n);
    if(nullfd > 2)
      close_e(nullfd);
  }

  /* create a new session */
  setsid_e();

  /* now we're a session leader.  But this is bad, as it means if we
   * ever open a tty device, it'll become our controlling terminal,
   * and we'll be liable to SIGHUPs from it. */

  if(fork_e())
    /* parent - terminate immediately */
    _exit(0);

  /* child - wait for the pipe to close so that we know when we can
   * restore SIGHUP */
  {
    char buffer[1];
    close_e(pipefds[1]);
    do {
      n = read(pipefds[0], buffer, 1);
    } while(n < 0 && errno == EINTR);
    if(n < 0)
      fatale("error calling read");
    close_e(pipefds[0]);
    sigaction_e(SIGHUP, &osa, 0);
  }

  /* now we are detached */

  /* now all that's left to do is execute the daemon program itself */
  execvp(argv[optind], argv + optind);

  fatale("error calling execvp %s", argv[optind]);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
