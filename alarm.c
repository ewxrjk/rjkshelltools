/*

   alarm - run a command with a timeout

   Copyright (C) 2001 Richard Kettlewell

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
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "utils.h"

static int verbose;			/* verbose mode */

/* Option flags and variables */

static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "signal", required_argument, 0, 's' },
  { "verbose", no_argument, 0, 'v' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fprintf(fp, "Usage:\n"
	     "\n"
	     "  alarm [options] -- timeout command ...\n"
	     "\n"
	     "Options:\n"
	     "\n"
	     "  -s SIGNAL, --signal SIGNAL       Signal to send program on expiry\n"
	     "  -v, --verbose                    Verbose mode\n"
	     "\n") < 0)
    fatale("output error");
  exit(exit_status);
}

/* return a+p*q - supposed to do range checking too, but we don't
 * bother yet! */

static int mla(int a, int p, int q) {
  return a + p * q;
}

/* parse a time specification and return its meaning in seconds */

static int parse_timeout(const char *s) {
  const char *str = s;
  int n = 0;
  int c;
  int t = 0;
  int l = 0;

  while((c = (unsigned char)*s++)) {
    int scale;

    if(isdigit(c)) {
      t = 10 * t + c - '0';
      ++l;
      continue;
    }
    switch(c) {
    case 's':
    case 'S':
      scale = 1;
      break;
    case 'm':
    case 'M':
      scale = 60;
      break;
    case 'h':
    case 'H':
      scale = 3600;
      break;
    case 'd':
    case 'D':
      scale = 86400;
      break;
    default:
      fatal("unknown time unit '%c' in '%s'", c, str);
    }
    if(!l)
      fatal("invalid time specification '%s'", str);
    n = mla(n, t, scale);
    l = 0;
    t = 0;
  }
  n = mla(n, t, 1);
  return n;
}

/* convert a signal name into a number */

static int parse_signal(const char *s) {
  const char *str = s;
  int n;

  if(strspn(s, "0123456789") == strlen(s))
    return atoi(s);
  /* the initial SIG is optional */
  if(!strncasecmp(s, "SIG", 3))
    s += 3;
  if((n = lookupi(signallookup, s)) >= 0)
    return n;
  fatal("unrecognized signal '%s'", str);
}

static pid_t pid;
static int child_killer_signal = SIGTERM;

static void alarm_handler(int __attribute__((unused)) sig) {
  /* timed out */
  if(verbose)
    error("timing out child process");
  if(kill(-pid, child_killer_signal) < 0)
    fatale("error sending signal %d to process group %lu",
	   child_killer_signal, (unsigned long)pid);
  /* let child_handler tidy up the child */
}

static void child_handler(int __attribute__((unused)) sig) {
  /* child terminated */
  int w;
  pid_t r;

  r = waitpid_e(pid, &w, WNOHANG);
  if(r == pid) {
    if(WIFEXITED(w)) {
      if((w == 0 && verbose > 1)
	 || (w != 0 && verbose > 0))
	fprintf(stderr, "%s: child %s\n", program_name, wstat(w));
      exit(w);
    } else {
      if(verbose)
	fprintf(stderr, "%s: child %s\n", program_name, wstat(w));
      exit(WTERMSIG(w) + 128);
    }
  }
}

int main(int argc, char **argv) {
  int n;
  int timeout;

  /* shut off any existing alarm, it will merely confuse matters */
  alarm(0);
  setprogname(argv[0]);

  while((n = getopt_long(argc, argv,
			 "hVs:v",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("alarm %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);
      return 0;

    case 's':
      child_killer_signal = parse_signal(optarg);
      break;

    case 'v':
      ++verbose;
      break;
      
    default:
      usage(stderr, 1);
    }

  }

  /* get the timeout */
  if(optind >= argc)
    fatal("no timeout speciifed");
  timeout = parse_timeout(argv[optind++]);

  if(optind >= argc)
    fatal("no command specified");

  /* set up signals */
  handle_signal(SIGALRM, alarm_handler);
  handle_signal(SIGCHLD, child_handler);

  /* actually install the handlers.  This needs to happen before the
   * fork as otherwise the signal mask may let an unhandled SIGCHLD
   * through after the fork but before we install a handler.  We'll
   * call restore_signals() inside the fork to put things back
   * right. */
  init_signals();
  
  /* create the child process */
  if(!(pid = fork_e())) {
    exiter = _exit;
    restore_signals();
    /* set our process group */
    if(setpgid(0, 0) < 0)
      fatale("error calling setpgrp");
    /* execute the command */
    execvp(argv[optind], argv + optind);
    fatale("error executing %s", argv[optind]);
  }

  /* Set the child's process group.  We do this in both the child so
   * that if the child forks again after the exec but before the
   * parent gets to run again the process group is right, and in the
   * parent so that if the parent gets to run first and sends a signal
   * to the child, the process group is already set. */
  if(setpgid(pid, pid) < 0)
    fatale("error calling setpgid");

  /* set the alarm */
  alarm(timeout);

  /* wait for signals to come in */
  signal_loop();
 
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
