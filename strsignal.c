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

#if ! HAVE_STRSIGNAL

#include <string.h>
#include <signal.h>
#include <stdio.h>

#if ! SYS_SIGLIST_DECLARED
extern const char *const sys_siglist[];
#endif

#if ! defined NSIG && defined _NSIG
# define NSIG _NSIG
#endif

const char *strsignal(int n) {
  static char buffer[128];
  
  if(n < 1 || n > NSIG) {
    snprintf(buffer, sizeof buffer, "signal %d", n);
    return buffer;
  }
  return sys_siglist[n];    
}

#endif

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
