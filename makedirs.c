/* 
   This file is part of rjkshellutils, Copyright (C) 2001 Richard Kettlewell

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

void makedirs(const char *path, mode_t mode) {
  char *p, *q;
  struct stat sb;
  mode_t cmode = (mode == (mode_t)-1 ? 0777 : mode);

  /* if there's already something at this path, don't try to create
   * it */
  if(lstat(path, &sb) == 0)
    return;
  /* get a copy of the string we can fiddle with */
  q = p = xstrdup(path);
  /* make leading directories */
  while(*q) {
    if(*q == '/') {
      *q = 0;
      if(mkdir(p, cmode) == 0)
	if(mode != (mode_t)-1)
	  if(chmod(p, mode) < 0)
	    fatale("error calling chmod \"%s\"", p);
      *q = '/';
    }
    ++q;
  }
  /* make the target directory itself */
  if(mkdir(p, cmode) == 0)
    if(mode != (mode_t)-1)
      if(chmod(p, mode) < 0)
	fatale("error calling chmod \"%s\"", p);
  free(p);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
