/*
   This file is part of rjkshelltools
   Copyright (C) 2001, 2002 Richard Kettlewell.

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

#ifndef UTILS_H
#define UTILS_H

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>

/* All these functions call fatal/fatale on error, unless otherwise
 * specified. */

/* program name */
extern const char *program_name;

/* set program name according to argv[0] */
void setprogname(const char *v);

/* return the maximum file descriptor that might be open */
int maxfd(void);

/* make FD close-on-exec */
void cloexec(int fd);

/* make FD non-blocking (O_NONBLOCK rather than O_NDELAY) */
void nonblock(int fd);

/* report an error, printf-style */
void error(const char *, ...) __attribute__((format(printf, 1, 2)));

/* true if debug output is desired */
extern int debugging;

/* issue a debug message */
void debug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* report an error, printf-style, including the errno string */
void errore(const char *, ...) __attribute__((format(printf, 1, 2)));

/* report an error, printf-style, and terminate */
void __attribute__((noreturn)) fatal(const char *, ...)
    __attribute__((format(printf, 1, 2)));

/* report an error, printf-style, including the errno string, and
 * terminate */
void __attribute__((noreturn)) fatale(const char *, ...)
    __attribute__((format(printf, 1, 2)));

/* function fatal(), fatale() call to terminate - default is exit().
 * Sometimes it is appropriate to set this to _exit inside a fork (or
 * occasionally outside a fork).  The rule to follow is that there
 * should be at most one call to exit() per call to main(). */
extern void (*exiter)(int);

/* write all of BUFFER, despite short writes.  On error, return -1 and
 * set errno.  On success, return the number of bytes written.  If an
 * error occurs, the number of bytes actually written is lost. */
int writeall(int fd, const void *buffer, size_t nbytes);

/* malloc wrapper which calls error() and fatal() on error */
void *xmalloc(size_t);

/* realloc wrapper which calls error() and fatal() on error */
void *xrealloc(void *, size_t);

/* duplicate string S using xmalloc */
char *xstrdup(const char *s);

/* use xmalloc to create a string using the N characters starting at
 * S */
char *xmemdup(const char *s, size_t n);

/* return the concatenation of two strings, allocated with xmalloc */
char *xstrdupcat(const char *, const char *);

/* return the concatenation of three strings, allocated with xmalloc */
char *xstrdupcat3(const char *, const char *, const char *);

/* opaque type for file descriptor mapping lists
 *
 * The point of this type and the associated functions is to construct
 * a list of file descriptors which we plan to dup() onto some other
 * file descriptors, perhaps after a fork.  It might be the case that
 * some of the open file descriptors have the same numbers as the
 * targets, and fdmap_map() copes with this.
 *
 * Notes:
 *
 * 1) It is not supported to specify the same CURRENT more than once;
 * fdmap_map() could be written to cope with this, but the current
 * version isn't.  Work around this with dup().
 *
 * 2) It makes no sense to specify the same TARGET more than once, as
 * fdmap_map() would have to decide which one to honor.
 *
 * Therefore, these two cases are checked for and rejected by
 * fdmap_add(). */
struct fdmap;

/* add an FD to a mapping list.  CURRENT is the currently open FD;
 * TARGET is where we want it to end up. */
void fdmap_add(struct fdmap **fdsp, int current, int target);

/* activate an FD mapping list. */
void fdmap_map(struct fdmap *fds);

/* close all the FDs in a mapping list */
void fdmap_close(const struct fdmap *fds);

/* free memory used by a mapping list */
void fdmap_free(struct fdmap *fds);

/* split STR on character CH, returning an allocated array of pointers
 * to allocated strings, terminated by a null pointer */
char **split(const char *str, int ch);

/* return a string describing a wait status.  Uses a static buffer, so
 * not thread safe. */
const char *wstat(int w);

/* lookup table for strings */
struct lookuptable {
  const char *name; /* name, or 0 at end of list */
  int value;        /* value to return */
};

/* lookup table of signal names (excluding the "SIG" bit!) */
extern const struct lookuptable signallookup[];

/* return the first VALUE for which NAME=KEY in a lookup table, or -1
 * if none does.  lookupi() is the same but does case independent
 * comparison. */
int lookup(const struct lookuptable *l, const char *key);
int lookupi(const struct lookuptable *l, const char *key);

struct sockaddr_in; /* forward declaration */

/* convert an ADDRESS:PORT or PORT specification into a sockaddr_in.
 * PROTO should be "tcp" or "udp" for the service file lookup. */
void inetaddress(struct sockaddr_in *addr, const char *addrspec,
                 const char *proto);

/* work out the directory name of PATH and return it as an allocated
 * string */
char *dirname(const char *path);

/* make directory PATH, and all directories leading up to it.  If MODE
 * is -1 then just use 0777-umask, else make the directories have mode
 * MODE. */
void makedirs(const char *path, mode_t mode);

/* change to user USER, group GROUP and switch to root directory ROOT.
 * If GROUP is 0 then the default group for USER is used.  If ROOT is
 * 0 then the root directory is not changed. */
void setpriv(const char *user, const char *group, const char *root);

/* request that signal SIG be handled by function FUNCTION.  Setup for
 * signal_loop().  All calls to handle_signal() MUST be before the
 * first call to init_signals() or signal_loop(). */
void handle_signal(int sig, void (*function)(int));

/* initialize signals for signal_loop (optional - signal_loop will do
 * it for you if you don't call this).  Blocks all the handled signals
 * and installs handlers for them.  Safe to call more than once, as it
 * remembers what state the signals are in. */
void init_signals(void);

/* restore signals (e.g. inside a fork).  Restores the handlers for
 * the handled signals to their original settings, and unblocks
 * them. */
void restore_signals(void);

/* Calls init_signals().  Then call sigsuspend to wait for one of
 * these signals to happen, and call the callbacks registered by
 * handle_signal when the signals are delivered.  The callbacks are
 * called outside the signal handler, and with all handled signals
 * blocked. */
void __attribute__((noreturn)) signal_loop(void);

struct sockaddr;

/* Parse a socket description on a command line.  ARGP points to the
 * argument number to start processing from, and will be updated.
 * ARGC and ARGV should be the argument array e.g. as passed to main.
 *
 * ADDRP should point to a buffer for the socket address to be
 * returned.  It should be *LENP bytes long.  *LENP will be updated
 * with the actual size.
 *
 * PFP is used to return the protocol family, TYPEP the type and
 * PROTOP the protocol (currently this is always set to 0).
 */
void parse_socket_arg(int *argp, int argc, char **argv, struct sockaddr *addrp,
                      int *lenp, int *pfp, int *typep, int *protop);

/* Convert the socket address at ADDR, length LEN, to a string.  The string
 * is written into a static buffer.
 *
 * If RDNS is nonzero then time-consuming lookups may be made to
 * convert numbers to names. */
const char *socketprint(const struct sockaddr *addr, size_t len, int rdns);

/* Read a line from FP.  Allocate space in the heap for it and save it
 * as a null-terminated string, including the newline (if there is
 * one).  On error, returns a null pointer.  At end of file, returns a
 * null pointer (but if there is a partial line, i.e. one with no
 * newline, returns that.) */
char *get_line(FILE *fp);

/* Return a hash of string S */
unsigned long hash(const char *s);

/* Call open(), call fatale() on error */
int open_e(const char *path, int flags, mode_t mode);

/* Call close(), call fatale() on error */
void close_e(int fd);

/* Call dup2(), call fatale() on error */
void dup2_e(int oldfd, int newfd);

/* Call dup(), call fatale() on error */
int dup_e(int oldfd);

/* Call pipe(), call fatale() on error */
void pipe_e(int fds[2]);

/* Call sigaction(), call fatale() on error */
void sigaction_e(int signo, const struct sigaction *new, struct sigaction *old);

/* Call sigprocmask(), call fatale() on error */
void sigprocmask_e(int how, const sigset_t *new, sigset_t *old);

/* Call fork(), call fatale() on error */
pid_t fork_e(void);

/* Call fcntl(), call fatale() on error */
int fcntl_e(int fd, int cmd, long arg);

/* Call waitpid(), call fatale() on error */
pid_t waitpid_e(pid_t pid, int *statusp, int options);

/* Call setsid(), call fatale() on error */
void setsid_e(void);

#endif /* UTILS_H */

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
