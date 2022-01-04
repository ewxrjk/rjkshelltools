/*
   with-lock - run a program with a lock held

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>

#include "utils.h"

/* Option flags and variables */

static struct option const long_options[] = {{"help", no_argument, 0, 'h'},
                                             {"version", no_argument, 0, 'V'},
                                             {"fd", required_argument, 0, 'f'},
                                             {"exclusive", no_argument, 0, 'e'},
                                             {"shared", no_argument, 0, 's'},
                                             {"no-fork", no_argument, 0, 'F'},
                                             {0, 0, 0, 0}};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs("Usage:\n"
           "  with-lock [options] [--] path command ...\n"
           "\n"
           "Options:\n"
           "  -f N, --fd N                          File descriptor to use\n"
           "  -e, --exclusive                       Get an exclusive "
           "('writer') lock\n"
           "                                        (default)\n"
           "  -s, --shared                          Get a shared ('reader') "
           "lock\n"
           "  -t SECONDS, --timeout SECONDS         Set a timeout\n"
           "  -F, --no-fork                         Execute command directly "
           "(not in a\n"
           "                                        subprocess)\n"
           "  -h, --help                            Usage message\n"
           "  -V, --version                         Version number\n",
           fp)
     < 0)
    fatale("output error");
  exit(exit_status);
}

static void __attribute__((noreturn))
alarm_handler(int __attribute__((unused)) sig) {
  static const char msg[] = "with-lock: timed out\n";
  int rc;

  rc = write(2, msg, sizeof msg);
  (void)rc; /* quieten compiler */
  _exit(1);
}

int main(int argc, char **argv) {
  int n;
  int fd, desired_fd = -1;
  int type = F_WRLCK;
  int flags = O_WRONLY;
  int timeout = -1;
  mode_t mode = 0666;
  char *path;
  struct flock fl;
  int forkme = 1;
  pid_t pid;
  int w;

  setprogname(argv[0]);
  while((n = getopt_long(argc, argv, "hVf:est:F", long_options, (int *)0))
        >= 0) {
    switch(n) {
    case 'V': printf("with-lock %s\n", VERSION); return 0;

    case 'h': usage(stdout, 0); return 0;

    case 'f': desired_fd = atoi(optarg); break;

    case 'e':
      type = F_WRLCK;
      flags = O_WRONLY;
      break;

    case 's':
      type = F_RDLCK;
      flags = O_RDONLY;
      break;

    case 't': timeout = atoi(optarg); break;

    case 'F': forkme = 0; break;

    default: usage(stderr, 1);
    }
  }

  /* pick up the path */
  if(optind >= argc)
    fatal("no path specified");
  path = argv[optind++];

  /* insist that there's a command */
  if(optind >= argc)
    fatal("no command specified");

  /* open the file.  FD_CLOEXEC had better not be the default
   * anywhere! */
  fd = open_e(path, flags | O_CREAT, mode);

  /* implement --fd option */
  if(desired_fd >= 0 && desired_fd != fd) {
    dup2_e(fd, desired_fd);
    close_e(fd);
    fd = desired_fd;
  }

  /* set a timeout */
  if(timeout >= 0) {
    struct sigaction sa;

    sa.sa_handler = alarm_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction_e(SIGALRM, &sa, 0);
    alarm(timeout);
  }

  /* take the lock */
  fl.l_type = type;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;
  /* can't use fcntl_e, as we can't safely pass a pointer through a
   * long */
  if(fcntl(fd, F_SETLKW, &fl) < 0)
    fatale("error calling fcntl F_SETLKW");

  /* cancel the timeout (there is a small chance that we get the lock
   * and then immediately time out) */
  if(timeout >= 0)
    alarm(0);

  if(forkme) {
    if((pid = fork_e())) {
      /* parent */
      waitpid_e(pid, &w, 0);
      if(WIFEXITED(w))
        exit(w);
      if(WIFSIGNALED(w)) {
        /* the alternative would be to propagate the signal back
         * through ourselves but that might (a) lie about whether
         * there had been a coredump and (b) overwrite the coredump.
         * (b) is fixable by setting a ulimit but I can't see how to
         * solve (a). */
        fprintf(stderr, "%s: child terminated by signal %d (%s)%s\n",
                program_name, WTERMSIG(w), strsignal(WTERMSIG(w)),
                WCOREDUMP(w) ? " (core dumped)" : "");
        _exit(WTERMSIG(w) + 128);
      }
      fatal("incomprehensible wait status %#x", (unsigned)w);
    }
    /* child */
    exiter = _exit;
  }

  /* execute the command with the lock held */
  execvp(argv[optind], argv + optind);
  fatale("error executing %s", argv[optind]);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
