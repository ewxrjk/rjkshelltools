/* 
   This file is part of rjkshellutils, Copyright (C) 2001 Richard Kettlewell

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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "utils.h"

int maxfd(void) {
#if HAVE_SYSCONF
  int n = sysconf(_SC_OPEN_MAX);

  if(n == -1)
    fatale("error calling sysconf");
  return n;
#else
  return 1023;			/* XXX guess! */
#endif
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
