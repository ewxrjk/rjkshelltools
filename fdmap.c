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

#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

struct fdmap {
  struct fdmap *next;			/* next FD to redirect */
  int current;				/* where the required file is now */
  int target;				/* the FD we want it to be on */
};

void fdmap_add(struct fdmap **fdsp, int current, int target) {
  struct fdmap *f;

  for(f = *fdsp; f && target != f->target && current != f->current; f = f->next)
    ;
  if(f)
    fatal("ambiguous redirection [current=%d target=%d]", current, target);
  f = xmalloc(sizeof *f);
  f->next = *fdsp;
  f->current = current;
  f->target = target;
  *fdsp = f;
}

void fdmap_map(struct fdmap *fds) {
  struct fdmap *f;

  for(f = fds; f; f = f->next) {
    if(f->current != f->target) {
      struct fdmap *g;
      
      /* our target might already be in use, if so, dup the offending
       * FD out of the way */
      for(g = f->next; g && f->target != g->current; g = g->next)
	;
      if(g)
	g->current = dup_e(g->current);
      dup2_e(f->current, f->target);
      close_e(f->current);
    }
  }
}

void fdmap_close(const struct fdmap *fds) {
  const struct fdmap *f;

  for(f = fds; f; f = f->next)
    close(f->current);
}

void fdmap_free(struct fdmap *fds) {
  if(fds) {
    fdmap_free(fds->next);
    free(fds);
  }
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
