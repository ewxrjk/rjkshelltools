/* 
   This file is part of rjkshellutils.  Copyright (C) 2001 Richard Kettlewell

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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "utils.h"

void nonblock(int fd) {
  int n;

  n = fcntl_e(fd, F_GETFL, 0);
  if((n & O_NONBLOCK) == 0)
    fcntl_e(fd, F_SETFL, n | O_NONBLOCK);
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
