/*
 * This file is part of rjkshelltools
 * Copyright (C) 2002 Richard Kettlewell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <config.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "utils.h"

pid_t waitpid_e(pid_t pid, int *statusp, int options) {
  int w;
  pid_t r;

  if(!statusp)
    statusp = &w;
  do {
    r = waitpid(pid, statusp, options);
  } while(r < 0 && errno == EINTR);
  if(r < 0)
    fatale("error calling waitpid");
  return r;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/