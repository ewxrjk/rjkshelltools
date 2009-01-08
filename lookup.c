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
#include <sys/types.h>

#include "utils.h"

int lookup(const struct lookuptable *l, const char *key) {
  while(l->name) {
    if(!strcmp(l->name, key))
      return l->value;
    ++l;
  }
  return -1;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
