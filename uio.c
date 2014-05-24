/*
   This file is part of rjkshelltools, Copyright (C) 2001 Richard Kettlewell.

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

#include "uio.h"

#if ! HAVE_SYS_UIO_H
ssize_t readv(int fd, const struct iovec *vector, int count) {
  int total = 0;
  int n;

  for(n = 0; n < count; ++n) {
    int bytes = read(fd, vector[n].iov_base, vector[n].iov_len);

    if(bytes > 0) {
      total += bytes;
      if(bytes < vector[n].iov_len)
	return total;
    } else if(bytes == 0)
      return total;
    else {
      if(total == 0)
	return -1;
      else
	return total;
    }
  }
  return total;
}

ssize_t writev(int fd, const struct iovec *vector, int count) {
  int total = 0;
  int n;

  for(n = 0; n < count; ++n) {
    int bytes = write(fd, vector[n].iov_base, vector[n].iov_len);

    if(bytes > 0) {
      total += bytes;
      if(bytes < vector[n].iov_len)
	return total;
    else {
      if(total == 0)
	return -1;
      else
	return total;
    }
  }
  return total;
}
#endif

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
