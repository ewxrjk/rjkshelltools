/*
   adverbio - I/O redirection with adverbial commands

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
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "utils.h"

/* Option flags and variables */

static struct option const long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"stdout", required_argument, 0, 'o'},
    {"stdin", required_argument, 0, 'i'},
    {"stderr", required_argument, 0, 'e'},
    {"output", required_argument, 0, 'O'},
    {"input", required_argument, 0, 'I'},
    {"redirect", required_argument, 0, 'r'},
    {"close", required_argument, 0, 'C'},
    {"append", no_argument, 0, 'a'},
    {"clobber", no_argument, 0, 'c'},
    {"safe", no_argument, 0, 's'},
    {"sync", no_argument, 0, 'S'},
    {0, 0, 0, 0}};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fprintf(fp,
             "Usage:\n"
             "\n"
             "  adverbio [options] -- command...\n"
             "\n"
             "Options:\n"
             "\n"
             "  -o FILENAME, --stdout FILENAME        Redirect stdout\n"
             "  -e FILENAME, --stderr FILENAME        Redirect stderr\n"
             "  -i FILENAME, --stdin FILENAME         Redirect stdin\n"
             "  -O FD:FILENAME, --output FD:FILENAME  Redirect FD to a file\n"
             "  -I FD:FILENAME, --input FD:FILENAME   Redirect FD from a file\n"
             "  -r FD1:FD2, --redirect FD1:FD2        Redirect FD1 to FD2\n"
             "  -C FD, --close FD                     Close FD\n"
             "  -a, --append                          Output in append mode\n"
             "  -c, --clobber                         Output in clobber mode\n"
             "  -s, --safe                            Output in safe mode\n"
#if O_SYNC
             "  -S, --sync                            Output synchronously\n"
#endif
             "  -h, --help                            Usage message\n"
             "  -V, --version                         Version number\n"
             "\n")
     < 0)
    fatale("output error");
  exit(exit_status);
}

/* redirect TARGET_FD to file PATH, using open() flags FLAGS */

static void redirect_to_file(int target_fd, const char *path, int flags) {
  int fd;

  fd = open_e(path, flags, 0666);
  if(fd != target_fd) {
    dup2_e(fd, target_fd);
    close_e(fd);
  }
}

/* parse a FD:FILENAME argument */

static void parse_fd_filename(const char *arg, int *fdp,
                              const char **filenamep) {
  int n = strspn(arg, "0123456789");

  if(n == 0 || arg[n] != ':')
    usage(stderr, 1);
  *fdp = atoi(arg);
  *filenamep = arg + n + 1;
}

/* parse a FD:FD argument */

static void parse_fd_fd(const char *arg, int *fd1p, int *fd2p) {
  int n = strspn(arg, "0123456789");

  if(n == 0 || arg[n] != ':'
     || arg[n + 1 + strspn(arg + n + 1, "0123456789")] != 0)
    usage(stderr, 1);
  *fd1p = atoi(arg);
  *fd2p = atoi(arg + n + 1);
}

int main(int argc, char **argv) {
  int n;
  int oflags = O_CREAT | O_EXCL;
  int syncflag = 0;
  int fd1, fd2;
  const char *filename;

  setprogname(argv[0]);

  while((n = getopt_long(argc, argv, "hVo:i:e:I:O:r:C:acsS", long_options,
                         (int *)0))
        >= 0) {
    switch(n) {
    case 'V': printf("adverbio %s\n", VERSION); return 0;

    case 'h': usage(stdout, 0); return 0;

    case 'o': redirect_to_file(1, optarg, O_WRONLY | oflags | syncflag); break;

    case 'e': redirect_to_file(2, optarg, O_WRONLY | oflags | syncflag); break;

    case 'i': redirect_to_file(0, optarg, O_RDONLY); break;

    case 'I':
      parse_fd_filename(optarg, &fd1, &filename);
      redirect_to_file(fd1, filename, O_RDONLY);
      break;

    case 'O':
      parse_fd_filename(optarg, &fd1, &filename);
      redirect_to_file(fd1, filename, O_WRONLY | oflags | syncflag);
      break;

    case 'r':
      parse_fd_fd(optarg, &fd1, &fd2);
      dup2_e(fd2, fd1);
      break;

    case 'C':
      if(!*optarg || optarg[strspn(optarg, "0123456789")] != 0)
        usage(stderr, 1);
      close_e(atoi(optarg));
      break;

    case 'c': oflags = O_CREAT | O_TRUNC; break;

    case 'a': oflags = O_CREAT | O_APPEND; break;

    case 's': oflags = O_CREAT | O_EXCL; break;

    case 'S':
#ifdef O_SYNC
      syncflag = O_SYNC;
#else
      error("warning: -S not supported on this platform");
#endif
      break;

    default: usage(stderr, 1);
    }
  }

  if(optind >= argc)
    usage(stderr, 1);

  execvp(argv[optind], argv + optind);

  fatale("error executing %s", argv[optind]);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
