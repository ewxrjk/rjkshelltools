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

#if HAVE_SYS_UIO_H
#include <sys/uio.h>
#else
struct iovec {
  void *iov_base;
  size_t iov_len;
};

ssize_t readv(int fd, const struct iovec *vector, int count);
ssize_t writev(int fd, const struct iovec *vector, int count);
#endif

#ifndef IOV_MAX
#ifdef UIO_MAXIOV
#define IOV_MAX UIO_MAXIOV
#else
#define IOV_MAX 16
#endif
#endif

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
