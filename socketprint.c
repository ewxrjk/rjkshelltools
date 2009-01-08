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

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utils.h"

const char *socketprint(const struct sockaddr *addr,
			size_t len,
			int rdns) {
  static char buffer[1024];
  int n;
  
  switch(addr->sa_family) {
  case AF_INET: {
    const struct sockaddr_in *in = (const struct sockaddr_in *)addr;
    struct hostent *he;

    if(rdns && (he = gethostbyaddr((const char *)&in->sin_addr,
				   sizeof in->sin_addr,
				   AF_INET))) {
      n = snprintf(buffer, sizeof buffer, "%s [%s]:%d",
		   he->h_name, inet_ntoa(in->sin_addr), ntohs(in->sin_port));
    } else {
      n = snprintf(buffer, sizeof buffer, "%s:%d",
		   inet_ntoa(in->sin_addr), ntohs(in->sin_port));
    }
    break;
  }
    
  case AF_UNIX: {
    const struct sockaddr_un *un = (const struct sockaddr_un *)addr;
    int l;

    if(!memchr(un->sun_path, 0,
	       len - offsetof(struct sockaddr_un, sun_path)))
      l = len - offsetof(struct sockaddr_un, sun_path);
    else
      l = strlen(un->sun_path);
    n = snprintf(buffer, sizeof buffer, "%.*s", l, un->sun_path);
    break;
  }
    
  default:
    n = snprintf(buffer, sizeof buffer, "unknown address family %#lx",
		 (unsigned long)addr->sa_family);
    break;
  }
  if(n < 0)
    fatale("error calling snprintf");
  return buffer;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
