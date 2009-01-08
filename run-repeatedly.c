/* 
   run-repeatedly - run and restart programs

   Copyright (C) 2001, 2003 Richard Kettlewell

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
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "utils.h"

struct daemon {
  struct daemon *next;			/* next daemon */
  char *path;				/* path */
  pid_t pid;				/* process ID or -1 */
  int present;				/* true if still present */
  int shutting_down;			/* true if already sent a
					 * shutdown signal */
  time_t last;				/* last start time */
};

static struct daemon *daemons;		/* list of managed daemons */
static int rescan_interval = 5*60;	/* interval between rescans */
static int execute_interval = 1*60;	/* interval between executions */
static const char *directory;		/* directory of daemons */
static int shutting_down;		/* shutting down? */
static int shutdown_obsolete;		/* terminate obsolete children */

/* option flags and variables */

static struct option const long_options[] =
{
  { "rescan-interval", required_argument, 0, 'i' },
  { "execute-interval", required_argument, 0, 'e' },
  { "shutdown", no_argument, 0, 's' },
  { "debug", no_argument, 0, 'd' },
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  run-repeatedly [options] [--] directory\n"
"\n"
"Options:\n"
"  -r SECONDS, --rescan-interval SECONDS   Set rescan interval\n"
"  -e SECONDS, --execute-interval SECONDS  Set execution interval\n"
"  -s, --shutdown                          Shut down children on removal\n"
"  -h, --help                              Usage message\n"
"  -V, --version                           Version number\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

/* limit events to once per INTERVAL seconds.  TIMEP should point to a
 * time_t which will be used to track when events occur; it should be
 * initialized to 0 before the first call.
 *
 * If the event is allowed, *TIMEP will be updated and 0 will be
 * returned.
 *
 * If the event is not allowed yet, 1 will be returned.
 */

static int ratelimit(int interval, time_t *timep) {
  time_t now;

  time(&now);
  if(*timep == 0
     || now >= *timep + interval) {
    *timep = now;
    return 0;
  } else {
    debug("rate limited");
    return 1;
  }
}

/* rescan the directory, return nonzero if there is a serious error */

static int rescan(void) {
  DIR *dp;
  struct dirent *de;
  struct daemon *d;
  
  static time_t last;

  debug("rescanning");
  if(ratelimit(rescan_interval, &last))
    return 0;
  
  for(d = daemons; d; d = d->next)
    d->present = 0;
  if(!(dp = opendir(directory))) {
    errore("error opening directory %s", directory);
    return -1;
  }
  while((de = readdir(dp))) {
    int i;
    char *fullpath;
    struct stat sb;
    struct daemon *d;

    for(i = 0; de->d_name[i]; ++i)
      if(!(isalnum((unsigned char)de->d_name[i])
	   || de->d_name[i] == '_'
	   || de->d_name[i] == '-'))
	break;
    if(de->d_name[i])
      continue;
    fullpath = xstrdupcat3(directory, "/", de->d_name);
    if(stat(fullpath, &sb) < 0) {
      errore("cannot stat %s", fullpath);
      free(fullpath);
      continue;
    }
    if(!S_ISREG(sb.st_mode)
       || ((sb.st_mode & 0111) == 0))
      continue;
    for(d = daemons; d; d = d->next)
      if(!strcmp(d->path, fullpath))
	break;
    if(!d) {
      d = xmalloc(sizeof *d);
      d->next = daemons;
      d->path = fullpath;
      d->pid = -1;
      d->present = 1;
      d->last = 0;
      d->shutting_down = 0;
      daemons = d;
    } else {
      d->present = 1;
      free(fullpath);
    }
  }
  closedir(dp);
  if(debugging)
    for(d = daemons; d; d = d->next)
      if(d->present)
	debug("found %s", d->path);
  return 0;
}

/* restart daemon D */

static void restart(struct daemon *d) {
  pid_t pid;
  sigset_t ss;

  /* don't start the daemon more than once per minute */
  debug("restarting %s", d->path);
  if(ratelimit(execute_interval, &d->last))
    return;
  switch(pid = fork()) {
  case -1:
    errore("[%s] error calling fork", d->path);
    break;
  case 0:
    alarm(0);
    exiter = _exit;
    /* re-enable signals */
    sigemptyset(&ss);
    sigprocmask_e(SIG_SETMASK, &ss, 0);
    if(setpgid(0, 0) < 0)
      fatale("[%s] error calling setpgid", d->path);
    execl(d->path, d->path, (const char *)0);
    fatale("error executing %s", d->path);
  default:
    d->pid = pid;
    break;
  }
}

/* start any daemons that aren't running */

static void startall(void) {
  struct daemon *d;

  if(!shutting_down) {
    /* restart any dead daemons */
    for(d = daemons; d; d = d->next)
      if(d->present && d->pid == (pid_t)-1)
	restart(d);
  }
}

/* clean up any dead, unwanted daemons */

static void cleanup(void) {
  struct daemon **dd, *d;

  dd = &daemons;
  while((d = *dd)) {
    if(d->pid == (pid_t)-1 && (!d->present || shutting_down)) {
      debug("cleaning up %s", d->path);
      *dd = d->next;
      free(d->path);
      free(d);
    } else
      dd = &d->next;
  }
}

static void alarm_handler(int __attribute__((unused)) sig) {
  debug("SIGALRM");
      
  /* spot changes to the control directory */
  rescan();
  if(shutdown_obsolete) {
    /* zap any obsolete daemons */
    struct daemon *d;

    for(d = daemons; d; d = d->next)
      if(d->pid != (pid_t)-1
	 && !d->present
	 && !d->shutting_down) {
	debug("shutting down %s", d->path);
	if(kill(d->pid, SIGTERM) < 0)
	  errore("error sending signal %d (%s) to %s[%lu]",
		 SIGTERM, strsignal(SIGTERM),
		 d->path, (unsigned long)d->pid);
	d->shutting_down = 1;
      }
  }
  /* start any children that have terminated */
  startall();
  /* cleanup memory allocation for obsolete daemons */
  cleanup();
  /* set another alarm */
  alarm(1);
}

static void child_handler(int __attribute__((unused)) sig) {
  pid_t p;
  int w;

  debug("SIGCHLD");

  /* pick up any terminated children */
  while((p = waitpid(-1, &w, WNOHANG)) > 0) {
    struct daemon *d;
    
    /* find which child terminated, if any */
    for(d = daemons; d && d->pid != p; d = d->next)
      ;
    if(d) {
      error("%s[%lu]: %s", d->path, (unsigned long)p, wstat(w));
      d->pid = -1;
    } else
      error("unknown process %lu: %s", (unsigned long)p, wstat(w));
  }
  if(p < 0) {
    switch(errno) {
    case EINTR:
    case ECHILD:
      debug("ignored error calling waitpid");
      break;
    default:
      errore("error calling waitpid");
      break;
    }
  } else if(p == 0)
    debug("waitpid returned 0");
  /* restart any terminated children */
  cleanup();
  startall();
  /* terminate if we reaped the last child */
  if(daemons == 0 && shutting_down) {
    sigset_t ss;

    debug("terminating myself");
    if(signal(shutting_down, SIG_DFL) == SIG_ERR)
      fatale("error calling signal");
    if(kill(getpid(), shutting_down) < 0)
      fatale("error sending signal %d (%s) to self [%lu]",
	     shutting_down, strsignal(shutting_down),
	     (unsigned long)getpid());
    sigemptyset(&ss);
    sigaddset(&ss, shutting_down);
    sigprocmask_e(SIG_UNBLOCK, &ss, 0);
    fatal("unexpectedly failed to terminate on signal %d to self",
	  shutting_down);
  }
}

static void shutdown_handler(int sig) {
  struct daemon *d;
  int s;
  
  /* record which signal we actually caught */
  shutting_down = sig;
  debug("%s", strsignal(shutting_down));
  /* which signal to actually send to children.  We substitute
   * SIGTERM for SIGINT as some programs e.g. bash don't seem to
   * like to terminate on SIGINT. */
  s = shutting_down == SIGINT ? SIGTERM : shutting_down;
  /* send the signal */
  for(d = daemons; d; d = d->next)
    if(d->pid != (pid_t)-1
       && !d->shutting_down) {
      debug("sending signal %d to %lu",
	    s, (unsigned long)d->pid);
      if(kill(d->pid, s) < 0)
	errore("error sending signal %d (%s) to %s[%lu]",
	       s, strsignal(s),
	       d->path, (unsigned long)d->pid);
    }
  /* turn off periodic rescans */
  alarm(0);
  /* do a speculative waitpid */
  child_handler(0);
}

int main(int argc, char **argv) {
  int n;

  setprogname(argv[0]);

  while((n = getopt_long(argc, argv, 
			 "hVr:e:ds",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("run-repeatedly %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);
      return 0;

    case 'r':
      rescan_interval = atoi(optarg);
      break;

    case 'e':
      execute_interval = atoi(optarg);
      break;

    case 'd':
      ++debugging;
      break;

    case 's':
      shutdown_obsolete = 1;
      break;
      
    default:
      usage(stderr, 1);
    }
  }

  /* pick up the directory */
  if(optind >= argc)
    fatal("no directory specified");
  directory = argv[optind++];

  if(optind < argc)
    fatal("unexpected extra arguments");

  /* set up signal handling */
  handle_signal(SIGCHLD, child_handler);
  handle_signal(SIGALRM, alarm_handler);
  handle_signal(SIGTERM, shutdown_handler);
  handle_signal(SIGINT, shutdown_handler);
  handle_signal(SIGHUP, shutdown_handler);
  
  /* perform initial scan */
  if(rescan())
    exit(-1);

  /* start up daemons */
  startall();

  /* set up alarms */
  alarm(1);

  /* enter event loop */
  signal_loop();
  
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
