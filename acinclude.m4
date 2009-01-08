dnl 
dnl This file is part of rjkshelltools
dnl Copyright (C) 2001, 2002 Richard Kettlewell
dnl 
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software Foundation,
dnl Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
dnl 

AC_DEFUN(RJK_C_GCC_ATTRIBUTES,[
  AC_CACHE_CHECK([for GNU C attribute syntax], rjk_cv_c_gcc_attributes, [
    AC_TRY_COMPILE([
static int foo() __attribute__((unused));
static int foo(int __attribute__((unused)) argc,
               char __attribute__((unused)) *argv[]) {
    return 0;
}],[return 0;],
      [rjk_cv_c_gcc_attributes=yes],
      [rjk_cv_c_gcc_attributes=no])
 ])
 if test "${rjk_cv_c_gcc_attributes}" = yes; then
   AC_DEFINE(HAVE_GCC_ATTRIBUTES, 1, [define if you have GNU C __attribute__ syntax])
 fi
])

AC_DEFUN(RJK_STRSIGNAL, [
  AC_CACHE_CHECK([for strsignal],
  rjk_cv_strsignal, [
    AC_EGREP_CPP(strsignal,[
      #include <string.h>
      #include <signal.h>
    ], [
      rjk_cv_strsignal=yes
    ], [
      AC_EGREP_CPP(strsignal,[
	#define _GNU_SOURCE
	#include <string.h>
	#include <signal.h>
      ], [
	rjk_cv_strsignal=gnu
      ], [
	rjk_cv_strsignal=no
      ])
    ])
  ])
  if test $rjk_cv_strsignal = gnu; then
    AC_DEFINE(_GNU_SOURCE, 1, [define if needed to get strsignal])
    AC_DEFINE(HAVE_STRSIGNAL, 1, [define if you have strsignal])
  elif test $rjk_cv_strsignal = yes; then
    AC_DEFINE(HAVE_STRSIGNAL, 1, [define if you have strsignal])
  else
    AC_DECL_SYS_SIGLIST
    AC_LIBOBJ="${AC_LIBOBJ} strsignal.o"
  fi
])

dnl Define HAVE_LONG_AF_UNIX_SOCKETS if AF_UNIX sockets are allowed to
dnl have names longer than sizeof sun_path.  sun_path is usually ridiculously
dnl small and at least some systems don't allow you to extend beyond it,
dnl however hopefully there are some out there that operate a more flexible
dnl policy.
AC_DEFUN(RJK_LONG_AF_UNIX_SOCKETS, [
  AC_CACHE_CHECK([whether long AF_UNIX names work],
  rjk_cv_long_af_unix_sockets, [
    AC_TRY_RUN([
      #include <sys/socket.h>
      #include <sys/un.h>
      #include <string.h>
      #include <stdio.h>
      #include <stddef.h>
      #include <errno.h>
      
      int main(void) {
        union {
          struct sockaddr sa;
          char padding[sizeof (struct sockaddr_un) + 64];
          struct sockaddr_un un;
        } u;
        char *ptr = u.un.sun_path;
        int fd;
        int len;
        int status;

        /* first check that we can bind to short names */
        if((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
          perror("socket");
          return 2;
        }
        memset(&u, 0, sizeof u);
        u.un.sun_family = AF_UNIX;
        strcpy(u.un.sun_path, "conftestsocket"); 
        if(bind(fd, &u.sa, sizeof u.un) < 0) {
          perror("bind");
          return 2;
        }
        close(fd);
        remove(u.un.sun_path);
       
        /* now try with a long one */
        if((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
          perror("socket");
          return 2;
        }
        memset(&u, 0, sizeof u);
        u.un.sun_family = AF_UNIX;
        strcpy(u.un.sun_path, "conftestsocket"); 
        memset(ptr + 14, 'A', sizeof u.un.sun_path + 2 - 14);
        ptr[sizeof u.un.sun_path + 2] = 0;
        len = offsetof(struct sockaddr_un, sun_path) + strlen(ptr) + 1;
        if(bind(fd, &u.sa, len) < 0)
          return 1;
        remove(u.un.sun_path);
        return 0;
      }
    ], [
      rjk_cv_long_af_unix_sockets=yes
    ], [
      rjk_cv_long_af_unix_sockets=no
    ], [
      # if cross-compiling, play it safe
      rjk_cv_long_af_unix_sockets=no
    ])
  ])
  if test "x$rjk_cv_long_af_unix_sockets" = xyes; then
    AC_DEFINE(HAVE_LONG_AF_UNIX_SOCKETS, 1, [define if you have long AF_UNIX paths])
  fi
])
