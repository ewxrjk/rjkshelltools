/* 
   This file is part of rjkshellutils.  Copyright (C) 2001 Richard Kettlewell

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
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>

#include "utils.h"

const char *wstat(int w) {
  static char buffer[1024];
  int n;

  if(WIFEXITED(w))
    n = snprintf(buffer, sizeof buffer, "exited with status %d",
		 WEXITSTATUS(w));
  else if(WIFSIGNALED(w))
    n = snprintf(buffer, sizeof buffer, "terminated by signal %d [%s]%s",
		 WTERMSIG(w), strsignal(WTERMSIG(w)),
		 WCOREDUMP(w) ? " (core dumped)" : "");
  else if(WIFSTOPPED(w))
    n = snprintf(buffer, sizeof buffer, "stopped by signal %d [%s]",
		 WTERMSIG(w), strsignal(WTERMSIG(w)));
  else
    n = snprintf(buffer, sizeof buffer, "unknown wait status %#x", (unsigned)w);
  if(n < 0)
    fatale("error calling snprintf");
  return buffer;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
