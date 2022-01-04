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

#include <string.h>
#include <sys/types.h>

#include "utils.h"

char **split(const char *str, int ch) {
  char **v = 0;
  int vs = 0;
  int n = 0;
  const char *ptr = str;

  while(*ptr) {
    const char *e, *next;

    if((e = strchr(ptr, ch)))
      next = e + 1;
    else
      next = e = ptr + strlen(ptr);
    if(n >= vs)
      v = xrealloc(v, sizeof(char *) * (vs = vs ? 2 * vs : 16));
    v[n++] = xmemdup(ptr, e - ptr);
    ptr = next;
  }
  if(n >= vs)
    v = xrealloc(v, sizeof(char *) * (vs = vs ? 2 * vs : 16));
  v[n] = 0;
  return v;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
