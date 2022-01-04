/*
   logfds - log output of a command

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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>

#include "utils.h"
#include "logdaemon.h"

/* Option flags and variables */
static struct option const long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"compress", no_argument, 0, 'c'},
    {"quiet", no_argument, 0, 'q'},
    {"max-log-age", required_argument, 0, 'm'},
    {"log-in-child", no_argument, 0, 'C'},
    {"day", required_argument, 0, 'D'},
    {0, 0, 0, 0}};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs("Usage:\n"
           "  logfds [options] [--] fd[,fd...] path ... [--] command ...\n"
           "\n"
           "Options:\n"
           "  -c, --compress                        Compress logs\n"
           "  -m DAYS, --max-log-age DAYS           Delete old logs\n"
           "  -q                                    Quiet mode\n"
           "  -C                                    Log in the child, not the "
           "parent\n"
           "  -h, --help                            Usage message\n"
           "  -V, --version                         Version number\n",
           fp)
     < 0)
    fatale("output error");
  exit(exit_status);
}

int main(int argc, char **argv) {
  int n;
  int max = 0;
  int compress = 0;
  struct fdmap *fds = 0;
  int quiet = 0;
  int loginchild = 0;
  pid_t pid;
  pid_t r;

  setprogname(argv[0]);

  while((n = getopt_long(argc, argv, "hVqcm:D:C", long_options, (int *)0))
        >= 0) {
    switch(n) {
    case 'V': printf("logfds %s\n", VERSION); return 0;

    case 'h': usage(stdout, 0);

    case 'm': max = atoi(optarg); break;

    case 'c': compress = 1; break;

    case 'q': ++quiet; break;

    case 'D': ld_day = atol(optarg); break;

    case 'C': loginchild = 1; break;

    default: usage(stderr, 1);
    }
  }

  /* process all redirections */
  while(optind < argc && isdigit(argv[optind][0])) {
    char **v;
    int p[2];
    struct logfile *l;
    int first = 1;

    /* check that there's a pattern */
    if(optind + 1 >= argc)
      fatal("missing filename pattern in redirection");
    /* create the pipe */
    pipe_e(p);
    /* make sure the input end is closed in the child */
    cloexec(p[0]);
    v = split(argv[optind], ',');
    if(!*v)
      fatal("empty file descriptor list in redirection");
    for(n = 0; v[n]; ++n) {
      int fd;

      if(!v[n][0] || v[n][strspn(v[n], "0123456789")])
        fatal("invalid file descriptor in redirection");
      /* we dup() file descriptors to keep the fdmap_* functions
       * happy */
      fd = first ? p[1] : dup_e(p[1]);
      fdmap_add(&fds, fd, atoi(v[n]));
      free(v[n]);
      first = 0;
    }
    free(v);
    ++optind;
    l = ld_new_logfile(argv[optind]);
    l->rotate = max;
    l->compress = compress;
    ld_new_input(p[0], l);
    ++optind;
  }

  /* skip optional separator */
  if(optind < argc && !strcmp(argv[optind], "--"))
    ++optind;

  /* check that there is a command */
  if(optind >= argc)
    fatal("no command to execute");

  /* launch the subprocess */
  pid = fork_e();
  if(loginchild ? pid != 0 : pid == 0) {
    exiter = _exit;
    /* redirect file descriptors */
    fdmap_map(fds);
    if(execvp(argv[optind], argv + optind) < 0)
      fatale("error executing %s", argv[optind]);
    fatal("execvp succeeded but returned");
  }

  /* close writer ends of pipes */
  fdmap_close(fds);
  fdmap_free(fds);

  /* enter the logger event loop */
  if(ld_loop())
    exit(-1);

  /* no more inputs; wait for the child to terminate */
  if(!loginchild) {
    r = waitpid_e(pid, &n, 0);
    if(!r)
      fatal("waitpid unexpectedly returned 0");

    /* report exit status if the child reported an exit status */
    if(n && !quiet)
      fprintf(stderr, "%s[%lu] %s\n", argv[optind], (unsigned long)pid,
              wstat(n));
  }

  /* pass an appropriate exit status to the caller */
  exit(WIFSIGNALED(n) ? 128 + WTERMSIG(n) : WEXITSTATUS(n));
}
