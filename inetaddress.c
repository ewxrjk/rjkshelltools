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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utils.h"

void inetaddress(struct sockaddr_in *addr,
		 const char *addrspec,
		 const char *proto) {
  char **v = split(addrspec, ':');
  const char *a, *p;

  /* check that there is exactly one colon */
  if(!v[0] || (v[1] && v[2]))
    fatal("invalid inet address \"%s\"", addrspec);
  if(v[1]) {
    a = v[0];
    p = v[1];
  } else {
    a = "0.0.0.0";
    p = v[0];
  }
  memset(addr, 0, sizeof *addr);
  addr->sin_family = AF_INET;
  /* convert the address */
  if(!inet_aton(a, &addr->sin_addr)) {
    struct hostent *he;

    if(!(he = gethostbyname(a)))
      fatal("error calling gethostbyname %s", a);
    memcpy(&addr->sin_addr, he->h_addr, sizeof (struct in_addr));
  }
  /* convert the port */
  if(strspn(p, "0123456789") == strlen(p))
    addr->sin_port = htons(atoi(p));
  else {
    struct servent *se;
    
    if(!(se = getservbyname(p, proto)))
      fatal("error calling getservbyname %s/%s", p, proto);
    addr->sin_port = se->s_port;
  }
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
