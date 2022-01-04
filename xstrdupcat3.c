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
#include <stdlib.h>
#include <string.h>
#include "utils.h"

char *xstrdupcat3(const char *s1, const char *s2, const char *s3) {
  size_t n1 = strlen(s1);
  size_t n2 = strlen(s2);
  size_t m = n1 + n2;
  char *ptr = xmalloc(m + strlen(s3) + 1);

  strcpy(ptr, s1);
  strcpy(ptr + n1, s2);
  strcpy(ptr + m, s3);
  return ptr;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
