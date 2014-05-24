/* 
   connect-socket - connect a socket and run a command

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>

#include "utils.h"

/* Option flags and variables */
static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  connect-socket [options] [--] fd [family] [type] address [--] command ...\n"
"\n"
"Options:\n"
"  -h, --help                    Usage message\n"
"  -V, --version                 Version number\n"
"\n"
"Families are \"inet\" or \"unix\".  Types are \"stream\" or \"dgram\".  For\n"
"internet sockets, addresses are \"addr:port\" or just \"port\".  For UNIX\n"
"sockets, the address is the path name.\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

int main(int argc, char **argv) {
  int n;
  int fdno;
  int fd;
  union {
    struct sockaddr sa;
    struct sockaddr_in in;
    struct sockaddr_un un;
  } u;
  int len, pf, type, proto;
  
  setprogname(argv[0]);
  
  while((n = getopt_long(argc, argv, 
			 "hV",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("connect-socket %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);
      
    default:
      usage(stderr, 1);
    }
  }

  if(optind >= argc
     || !isdigit((unsigned char)argv[optind][0]))
    fatal("no file descriptor specified");
    
  fdno = atoi(argv[optind++]);

  /* parse the socket name */
  len = sizeof u;
  parse_socket_arg(&optind, argc, argv, &u.sa,
		   &len, &pf, &type, &proto);

  /* create the socket */
  if((fd = socket(pf, type, proto)) < 0)
    fatale("error creating socket");
  
  
  if(connect(fd, &u.sa, len) < 0)
    fatale("error connecting socket");
  
  /* use the chosen FD */
  if(fd != fdno) {
    dup2_e(fd, fdno);
    close_e(fd);
  }

  /* skip separator */
  if(optind < argc && !strcmp(argv[optind], "--"))
    ++optind;
  
  /* check there's a command */
  if(optind >= argc)
    fatal("no command specified");
  
  /* execute the program */
  if(execvp(argv[optind], argv + optind) < 0)
    fatale("error executing %s", argv[optind]);
  fatal("execvp %s unexpectedly returned", argv[optind]);
}


/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
