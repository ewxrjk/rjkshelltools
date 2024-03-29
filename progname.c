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

#include <config.h>

#include <sys/types.h>
#include <string.h>

#include "utils.h"

/* program name */
const char *program_name;

void setprogname(const char *v) {
  if(!(program_name = strchr(v, '/')))
    program_name = v;
  else
    ++program_name;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
