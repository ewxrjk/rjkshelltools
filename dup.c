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

#include <unistd.h>
#include <fcntl.h>

#include "utils.h"

int dup_e(int oldfd) {
  int ret;
  
  if((ret = dup(oldfd) < 0))
    fatale("error calling dup");
  return ret;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
