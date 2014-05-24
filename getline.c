/*
 * This file is part of rjkshelltools
 * Copyright (C) 2002 Richard Kettlewell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdlib.h>

#include "utils.h"

char *get_line(FILE *fp) {
  char *ptr = 0;
  size_t space = 0, n = 0;
  int c;

  c = fgetc(fp);
  while(!ferror(fp) && !feof(fp)) {
    if(n >= space)
      ptr = xrealloc(ptr, space = space ? 2 * space : 64);
    ptr[n++] = c;
    if(c == '\n')
      break;
    else
      c = getc(fp);
  }
  if(ferror(fp)) {
    free(ptr);
    return 0;
  }
  if(!(feof(fp) && !ptr)) {
    if(n >= space)
      ptr = xrealloc(ptr, space = space ? 2 * space : 64);
    ptr[n++] = 0;
  }
  return ptr;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
