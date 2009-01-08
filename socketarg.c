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
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "utils.h"

/* lookup tables for socket types and families */
static const struct lookuptable families[] = {
  { "inet", AF_INET },
  { "unix", AF_UNIX },
  { 0, 0 }
};

static const struct lookuptable types[] = {
  { "stream", SOCK_STREAM },
  { "dgram", SOCK_DGRAM },
  { 0, 0 }
};

void parse_socket_arg(int *argp,
		      int argc, char **argv,
		      struct sockaddr *addrp,
		      int *lenp,
		      int *pfp, int *typep, int *protop) {
  int af, type;

  memset(addrp, 0, *lenp);
  
  if(*argp < argc
     && (af = lookup(families, argv[*argp])) != -1)
    ++*argp;
  else
    af = AF_INET;

  if(*argp < argc
     && (type = lookup(types, argv[*argp])) != -1)
    ++*argp;
  else
    type = SOCK_STREAM;

  switch(af) {
  case AF_INET:
    if(*argp >= argc)
      fatal("no address specified for inet socket");
    if((size_t)*lenp < sizeof (struct sockaddr_in))
      fatal("address buffer for parse_socket_arg too small");
    inetaddress((struct sockaddr_in *)addrp,
		argv[*argp], type == SOCK_STREAM ? "tcp" : "udp");
    /* report back the length */
    *lenp = sizeof (struct sockaddr_in);
    ++*argp;
    *pfp = PF_INET;
    break;
  case AF_UNIX: {
    struct sockaddr_un *su = (struct sockaddr_un *)addrp;
    int l = offsetof(struct sockaddr_un, sun_path) + strlen(argv[*argp]) + 1;

#if ! HAVE_LONG_AF_UNIX_SOCKETS
    if(l > (int)sizeof (struct sockaddr_un))
      fatal("path \"%s\" is too long for a UNIX domain socket", argv[*argp]);
#endif
    
    if(*argp >= argc)
      fatal("no address specified for unix socket");
    /* check length */
    if(l > *lenp)
      fatal("path \"%s\" is too long for the socket address buffer",
	    argv[*argp]);
    /* make the address */
    su->sun_family = AF_UNIX;
    strcpy(su->sun_path, argv[*argp]);	/* checked above */
    /* report back the length */
    *lenp = l;
    ++*argp;
    *pfp = PF_UNIX;
    break;
  }
    
  default:
    fatal("unknown address family %d", af);
  }
  
  *typep = type;
  *protop = 0;
    
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
