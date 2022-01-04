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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "utils.h"

int writeall(int fd, const void *buffer, size_t nbytes) {
  size_t written = 0;

  while(written < nbytes) {
    int n = write(fd, written + (const char *)buffer, nbytes - written);

    if(n < 0) {
      if(errno == EINTR)
        continue;
      return -1;
    }
    written += n;
  }
  return written;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
