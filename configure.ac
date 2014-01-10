dnl
dnl This file is part of rjkshelltools, Copyright (C) 2001 Richard Kettlewell
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
dnl dnl Process this file with autoconf to produce a configure script.

AC_INIT([rjkshelltools], [0.6.1], [richard+rjkshelltools@sfere.greenend.org.uk])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([inplace.c])
AM_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_AWK
AC_PROG_RANLIB

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h string.h sys/uio.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CACHE_CHECK([for in_addr_t], rjk_cv_in_addr_t, [
  AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <netinet/in.h>
  ], [
    in_addr_t x;
  ], [rjk_cv_in_addr_t=yes],
  [rjk_cv_in_addr_t=no])
])
if test "${rjk_cv_in_addr_t}" = no; then
  AC_DEFINE(in_addr_t, unsigned long, [define if you do not have in_addr_t])
fi
RJK_LONG_AF_UNIX_SOCKETS

dnl Checks for library functions.
AC_CHECK_FUNCS([sysconf])
AC_REPLACE_FUNCS([inet_aton])
RJK_STRSIGNAL

AC_CACHE_CHECK([for a word list], [rjk_cv_wordlist], [
  if test "x$WORDLIST" = x; then
    rjk_cv_wordlist=""
    for path in /usr/share/dict/words /usr/dict/words; do
      if test -f "$path"; then
	rjk_cv_wordlist="$path"
	break
      fi
    done
  else
    rjk_cv_wordlist="WORDLIST"
  fi
])
if test "x$rjk_cv_wordlist" = x; then
  AC_MSG_ERROR([no word list found, set WORDLIST and send a patch])
fi
AC_SUBST([WORDLIST], "$rjk_cv_wordlist")

dnl GNU C features
RJK_C_GCC_ATTRIBUTES

if test "$GCC" = yes; then
  CC="${CC} -Werror -Wall -W -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Wsign-compare -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wnested-externs"
fi

AH_BOTTOM([
#if ! HAVE_GCC_ATTRIBUTES
# define __attribute__(x)
#endif

#if ! HAVE_INET_ATON
in_addr_t inet_aton(const char *cp, struct in_addr *addr);
#endif

#if ! HAVE_STRSIGNAL
const char *strsignal(int);
#endif
])

AC_CONFIG_FILES([Makefile pidfile wordlist.h])
AC_CONFIG_COMMANDS([pidfile-x],[chmod +x pidfile])
AC_OUTPUT
