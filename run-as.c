/* 
   run-as - run a command as a specified user

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "utils.h"

/* Option flags and variables */
static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "root", required_argument, 0, 'r' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  run-as [options] -- user [group] -- command ...\n"
"\n"
"Options:\n"
"  -r DIRECTORY, --root DIRECTORY    Change root directory\n"
"  -h, --help                        Usage message\n"
"  -V, --version                     Version number\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

int main(int argc, char **argv) {
  int n;
  const char *root = 0;
  const char *user, *group;

  setprogname(argv[0]);
  
  while((n = getopt_long(argc, argv, 
			 "hVr:",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("run-as %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);

    case 'r':
      root = optarg;
      break;
      
    default:
      usage(stderr, 1);
    }
  }
  
  /* check for a user */
  if(optind >= argc)
    fatal("no user specified");
  user = argv[optind++];

  /* check for a group */
  if(optind < argc && strcmp(argv[optind], "--"))
    group = argv[optind++];
  else
    group = 0;

  /* check for the separator */
  if(optind >= argc || strcmp(argv[optind], "--"))
    fatal("missing command separator");
  ++optind;

  /* check there is a command */
  if(optind >= argc)
    fatal("missing command");

  /* do all the changes */
  setpriv(user, group, root);
  
  /* execute the command */
  execvp(argv[optind], argv + optind);
  fatale("error executing \"%s\"", argv[optind]);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
