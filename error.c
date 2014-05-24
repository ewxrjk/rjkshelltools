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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include "utils.h"

void errore(const char *fmt, ...) {
  va_list ap;
  int e = errno;

  fprintf(stderr, "%s: ", program_name);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, ": %s\n", strerror(e));
}

void error(const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "%s: ", program_name);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
