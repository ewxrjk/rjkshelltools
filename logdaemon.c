/* 
   This file is part of rjkshellutils.  Copyright (C) 2001 Richard Kettlewell

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
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <glob.h>
#define SYSLOG_NAMES
#include <syslog.h>
#include "utils.h"
#include "logdaemon.h"

#define SUSPEND_PERIOD 60		/* how long to suspend inputs for */
#define SELECT_RESOLUTION 10		/* upper limit to select
					 * timeout if there are
					 * suspended inputs */

struct logfile *ld_logfiles;		/* linked list of logfiles */
struct syslogfile *ld_syslogfiles;	/* linked list of syslogfiles */
struct input *ld_inputs;		/* linked list of inputs */
long ld_day = 86400;			/* seconds between rotations */

static fd_set fds;			/* input FD set for select */
static int maxinput = -1;		/* max selectable FD */
static int suspended;			/* number of suspended inputs */

/* block all signals, save the old signal mask via SS */

static void block(sigset_t *ss) {
  sigset_t all;

  sigfillset(&all);
  sigprocmask_e(SIG_BLOCK, &all, ss);
}

/* restore signals, using SS as above */

static void unblock(const sigset_t *ss) {
  sigprocmask_e(SIG_SETMASK, ss, 0);
}

struct logfile *ld_new_logfile(const char *pattern) {
  struct logfile *l;
  sigset_t ss;
  char *p;

  if(!(p = ld_globtime(pattern)))
    fatal("cannot convert strftime pattern '%s' to a glob pattern", pattern);
  free(p);
  block(&ss);
  for(l = ld_logfiles; l; l = l->next)
    if(!strcmp(l->pattern, pattern)) {
      ++l->refs;
      return l;
    }
  l = xmalloc(sizeof *l);
  l->pattern = xstrdup(pattern);
  l->path = 0;
  l->fd = -1;
  l->date = -1;
  l->rotate = 0;
  l->compress = 0;
  l->usegmt = 1;
  l->buffer = 0;
  l->bufsize = 0;
  l->next = ld_logfiles;
  l->refs = 1;
  ld_logfiles = l;
  unblock(&ss);
  return l;
}

void ld_delete_logfile(struct logfile *l) {
  struct logfile **ll;
  sigset_t ss;

  block(&ss);
  if(!--l->refs) {
    for(ll = &ld_logfiles;
	*ll && *ll != l;
	ll = &(*ll)->next)
      ;
    if(*ll)
      *ll = l->next;
    if(l->fd != -1)
      close(l->fd);
    free(l->pattern);
    free(l->path);
    free(l->buffer);
    free(l);
  }
  unblock(&ss);
}

static int decodename(const CODE *names, const char *name, size_t namelen) {
  while(names->c_name) {
    if(strlen(names->c_name) == namelen
       && !strncmp(names->c_name, name, namelen))
      return names->c_val;
    ++names;
  }
  return -1;
}

struct syslogfile *ld_new_syslogfile(const char *facpri) {
  struct syslogfile *l;
  sigset_t ss;
  const char *pri;
  size_t faclen;
  int prino, facno;

  if((pri = strchr(facpri, '.'))) {
    faclen = pri - facpri;
    ++pri;
  } else {
    faclen = strlen(facpri);
    pri = "info";
  }
  if((facno = decodename(facilitynames, facpri, faclen)) < 0
     || (prino = decodename(prioritynames, pri, strlen(pri))) < 0)
    return 0;
  block(&ss);
  l = xmalloc(sizeof *l);
  l->pri = prino;
  l->fac = facno;
  l->buffer = 0;
  l->bufsize = 0;
  l->buffer = 0;
  l->next = ld_syslogfiles;
  ld_syslogfiles = l;
  unblock(&ss);
  return l;
}

void ld_delete_syslogfile(struct syslogfile *l) {
  struct syslogfile **ll;
  sigset_t ss;

  block(&ss);
  for(ll = &ld_syslogfiles;
      *ll && *ll != l;
      ll = &(*ll)->next)
    ;
  if(*ll)
    *ll = l->next;
  free(l);
  unblock(&ss);
}

struct input *ld_new_input(int fd, void *l) {
  struct input *i = xmalloc(sizeof *i);
  sigset_t ss;

  i->fd = fd;
  i->suspended = 0;
  i->log = l;
  i->input_callback = ld_input_callback;
  i->daily_callback = ld_daily_callback;
  block(&ss);
  if(i->fd != -1) {
    FD_SET(i->fd, &fds);
    if(maxinput != -1 && i->fd > maxinput)
      maxinput = i->fd;
  }
  i->next = ld_inputs;
  ld_inputs = i;
  unblock(&ss);
  return i;
}

void ld_delete_input(struct input *i) {
  struct input **ii;
  sigset_t ss;

  block(&ss);
  for(ii = &ld_inputs;
      *ii && *ii != i;
      ii = &(*ii)->next)
    ;
  if(*ii)
    *ii = i->next;
  if(i->fd != -1) {
    FD_CLR(i->fd, &fds);
    if(i->fd == maxinput)
      maxinput = -1;
  }
  unblock(&ss);
  if(i->fd != -1)
    close(i->fd);
  free(i);
}

void ld_suspend_input(struct input *i) {
  if(!i->suspended) {
    sigset_t ss;
    
    block(&ss);
    FD_CLR(i->fd, &fds);
    time(&i->suspended);
    i->suspended += SUSPEND_PERIOD;
    if(i->fd == maxinput)
      maxinput = -1;
    ++suspended;
    unblock(&ss);
  }
}

void ld_resume_input(struct input *i) {
  if(i->suspended) {
    sigset_t ss;
    time_t now;

    block(&ss);
    i->suspended = 0;
    FD_SET(i->fd, &fds);
    if(maxinput != -1 && i->fd > maxinput)
      maxinput = i->fd;
    --suspended;
    /* ld_loop calls back with signals blocked, so we do too */
    time(&now);
    (*i->input_callback)(i, now);
    unblock(&ss);
  }
}

int ld_open_logfile(struct logfile *l, time_t now) {
  char *newpath = 0;
  size_t size = PATH_MAX;
  struct tm *t;
  
  /* work out the filename we'll write to */
  t = (l->usegmt ? gmtime : localtime)(&now);
  /* keep expanding the buffer for the path until it's big enough */
  for(;;) {
    size_t len;

    newpath = xrealloc(newpath, size);
    len = strftime(newpath, size, l->pattern, t);
    if(len == 0)
      size *= 2;
    else
      break;
  }
  /* if the currently open filename is wrong, close it */
  if(l->path && strcmp(l->path, newpath))
    ld_close_logfile(l);
  /* if the is an open file it must now be the right one; so if there
   * is no file, open the newly chosen name. */
  if(!l->path) {
    /* try to open the file */
    if((l->fd = open(newpath, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0) {
      if(l->fd == -1 && errno == ENOENT) {
	char *ptr;
	/* this probably means the directory is missing, we attempt to
	 * create it */
	ptr = newpath;
	while(*ptr) {
	  if(*ptr == '/' && ptr != newpath) {
	    *ptr = 0;
	    mkdir(newpath, 0777);
	    *ptr = '/';
	  }
	  ++ptr;
	}
	/* try again after (possibly...) creating directories */
	if((l->fd = open(newpath, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0) {
	  errore("error opening %s", newpath);
	  free(newpath);
	  return -1;
	}
      }
    }
    /* record what path we've opened */
    l->path = xstrdup(newpath);
  }
  free(newpath);
  return 0;
}

void ld_close_logfile(struct logfile *l) {
  if(l->fd != -1) {
    close(l->fd);
    l->fd = -1;
    free(l->path);
    l->path = 0;
  }
}

time_t ld_next_daily(time_t now) {
  /* XXX the user might want midnight local time; we currently only
   * implement midnight GMT.  Patches invited, probably from people
   * who live outside GMT-land. */
  now += ld_day;
  now -= now % ld_day;
  return now;
}

static void daily(time_t now, time_t *next_daily) {
  struct input *i = ld_inputs;

  while(i) {
    struct input *inext = i->next;

    if(i->daily_callback)
      (*i->daily_callback)(i, now);
    i = inext;
  }
  *next_daily = ld_next_daily(now);
}

int ld_loop(void) {
  struct input *i;
  time_t now, next_daily;
  sigset_t ss;

  block(&ss);
  /* calculate fd set, which might not have been initialized
   * sensibly */
  FD_ZERO(&fds);
  for(i = ld_inputs; i; i = i->next)
    if(i->fd != -1)
      FD_SET(i->fd, &fds);
  unblock(&ss);
  /* work out when to next do rotations, etc */
  time(&now);
  next_daily = ld_next_daily(now);
  while(ld_inputs) {
    fd_set rfds;
    struct timeval tv;
    int n;

    time(&now);
    block(&ss);
    /* see if we need to rotate yet */
    if(now >= next_daily) {
      daily(now, &next_daily);
      time(&now);
    }
    /* unsuspend inputs */
    if(suspended) {
      for(i = ld_inputs; i; i = i->next) {
	if(i->suspended && i->suspended <= now) {
	  ld_resume_input(i);
	  if(!suspended)
	    break;
	}
      }
    }
    if(maxinput == -1) {
      /* recalculate maximum FD */
      for(i = ld_inputs; i; i = i->next)
	if(i->fd > maxinput)
	  maxinput = i->fd;
    }
    
    /* copy file descriptor set */
    rfds = fds;
    /* work out timeout */
    tv.tv_sec = next_daily - now;
    tv.tv_usec = 0;
    if(suspended && tv.tv_sec > SELECT_RESOLUTION)
      tv.tv_sec = SELECT_RESOLUTION;
    unblock(&ss);
    /* wait for something to happen */
    n = select(maxinput + 1, &rfds, 0, 0, &tv);
    if(n < 0) {
      if(errno == EINTR)
	continue;
      errore("select");
      return -1;			/* fatal error from select */
    }
    if(n) {
      block(&ss);
      /* see which inputs tripped */
      i = ld_inputs;
      while(i) {
	struct input *inext = i->next;
	if(i->fd != -1
	   && FD_ISSET(i->fd, &rfds)) {
	  (*i->input_callback)(i, now);
	  if(--n == 0)
	    break;
	}
	i = inext;
      }
      unblock(&ss);
    }
  }
  return 0;
}

void ld_input_callback(struct input *i, time_t now) {
  char buffer[4096];
  int bytes;
  struct logfile *l = i->log;
  int written;

  /* open the logfile */
  if(ld_open_logfile(l, now)) {
    ld_suspend_input(i);
    return;
  }
  /* flush buffered data */
  while(l->bufsize) {
    bytes = write(l->fd, l->buffer, l->bufsize);
    if(bytes < 0) {
      errore("error writing to %s", l->path);
      close(l->fd);
      l->fd = -1;
      free(l->path);
      l->path = 0;
      ld_suspend_input(i);
      return;
    }
    memmove(l->buffer, l->buffer + bytes, l->bufsize -= bytes);
    /* exercise for reader: more efficient handling of buffered
     * data */
  }
  /* free up the buffer if it has emptied */
  if(l->buffer && ! l->bufsize) {
    free(l->buffer);
    l->buffer = 0;
  }
  /* read some data */
  bytes = read(i->fd, buffer, sizeof buffer);
  if(bytes < 0) {
    /* we check EAGAIN, as sometimes we are called speculatively
     * rather than from the select loop */
    if(errno == EINTR || errno == EAGAIN)
      return;
    errore("error reading input stream");
    ld_delete_input(i);
    return;
  }
  if(bytes == 0) {
    /* end of file */
    ld_delete_input(i);
    return;
  }
  /* output the data */
  written = 0;
  while(written < bytes) {
    int n = write(l->fd, buffer + written, bytes - written);

    if(n < 0) {
      if(errno == EINTR)
	continue;
      errore("error writing to %s", l->path);
      break;
    }
    written += n;
  }
  if(bytes < written) {
    /* buffer any remaining data */
    l->buffer = xrealloc(l->buffer, l->bufsize + bytes - written);
    memcpy(l->buffer + l->bufsize, buffer + written, bytes - written);
    l->bufsize += bytes - written;
    /* close the logfile and come back to it in a while */
    ld_close_logfile(l);
    ld_suspend_input(i);
  }
}

void ld_syslog_callback(struct input *i, time_t __attribute__((unused)) now) {
  struct syslogfile *l = i->log;
  int bytes;
  char *nl;

  /* make sure there's some space in the buffer */
  if(l->bytes >= l->bufsize)
    l->buffer = xrealloc(l->buffer,
			 l->bufsize = l->bufsize ? 2 * l->bufsize : 128);
  bytes = read(i->fd, l->buffer, l->bufsize);
  if(bytes < 0) {
    /* we check EAGAIN, as sometimes we are called speculatively
     * rather than from the select loop */
    if(errno == EINTR || errno == EAGAIN)
      return;
    errore("error reading input stream");
    ld_delete_input(i);
    return;
  }
  if(bytes == 0) {
    /* end of file */
    if(l->bytes)
      syslog(l->pri | l->fac, "%.*s", (int)l->bytes, l->buffer);
    ld_delete_input(i);
    return;
  }
  l->bytes += bytes;
  while((nl = memchr(l->buffer, '\n', l->bytes))) {
    syslog(l->pri | l->fac, "%.*s", (int)(nl - l->buffer), l->buffer);
    /* XXX inefficient */
    ++nl;
    memmove(l->buffer, nl, (l->buffer + l->bytes - nl));
    l->bytes -= nl - l->buffer;
  }
}

void ld_daily_callback(struct input *i, time_t now) {
  struct logfile *l = i->log;
  char *pattern;
  glob_t g;
  size_t n;
  mode_t default_mode = (mode_t)-1;

  /* skip this logfile if we aren't supposed to do anything */
  if(!(l->rotate || l->compress))
    return;
  /* make sure the logfile is closed, so it doesn't hurt if we hit the
   * current one (which will presumably have become non-current) */
  ld_close_logfile(l);
  /* convert the strftime pattern to a glob pattern */
  pattern = ld_globtime(l->pattern);
  if(l->rotate) {
    /* get a list of all files */
    switch(glob(pattern, GLOB_NOSORT, 0, &g)) {
    case 0:
    case GLOB_NOMATCH:
      break;
    case GLOB_NOSPACE: goto nospace;
    case GLOB_ABORTED: goto readerror;
    }
    if(l->compress) {
      /* include compressed files */
      char *cpattern = xstrdupcat(pattern, ".gz");

      switch(glob(cpattern, GLOB_NOSORT|GLOB_APPEND, 0, &g)) {
      case 0:
      case GLOB_NOMATCH:
	break;
      case GLOB_NOSPACE:
	free(cpattern);
	goto nospace;
      case GLOB_ABORTED:
	free(cpattern);
	goto readerror;
      }
      free(cpattern);
    }
    /* now find any too-old files */
    for(n = 0; n < g.gl_pathc; ++n) {
      struct stat sb;
      char *path = g.gl_pathv[n];

      if(lstat(path, &sb) < 0)
	continue;
      /* only care about regular files */
      if(S_ISREG(sb.st_mode)
	 && sb.st_mtime < now - l->rotate * ld_day) {
	int m;
	
	if(unlink(path) < 0)
	  errore("removing %s", path);
	/* We want to unlink containing directories, but rmdir'ing all
	 * the way down to the root seems like a bad idea.  There are
	 * a number of safeguards for this:
	 *
	 * Firstly we don't remove any subdirectory that exactly
	 * matches the prefix of the pattern.  So if the original
	 * pattern was /var/log/%Y/%m/%d/foo.log then we will remove
	 * everything below /var/log, but not /var/log itself (even if
	 * we have permission).
	 *
	 * Secondly we only remove genuine directories - once we hit a
	 * symlink, we stop dead.  If the user introduces symlinks in
	 * the the middle of the path, it is up to them to clear up
	 * the results.
	 *
	 * Thirdly we only remove directories that have the same
	 * permissions as they would get if we re-created them.
	 */
	m = strlen(path);
	while(m > 0) {
	  if(path[m] == '/') {
	    path[m] = 0;
	    /* lexical checks */
	    if(!strncmp(path, pattern, m))
	      break;
	    /* make sure we know what mode we create files with */
	    if(default_mode == (mode_t)-1) {
	      mode_t u;
	      sigset_t ss;

	      block(&ss);
	      /* can't get the umask without changing it, argh */
	      u = umask(0777);
	      umask(u);
	      unblock(&ss);
	      default_mode = 0777 ^ u;
	    }
	    /* checks based on the file system */
	    if(lstat(path, &sb) < 0
	       || !S_ISDIR(sb.st_mode)
	       || (sb.st_mode & 0777) != default_mode
	       || sb.st_uid != geteuid()
	       || sb.st_gid != getegid())
	      break;
	    /* attempt the removal */
	    if(rmdir(path) < 0)
	      break;
	  }
	  --m;
	}
      }
    }
    globfree(&g);
  }
  if(l->compress) {
    /* get a list of all uncompressed files */
    switch(glob(pattern, GLOB_NOSORT, 0, &g)) {
    case 0:
    case GLOB_NOMATCH:
      break;
    case GLOB_NOSPACE: goto nospace;
    case GLOB_ABORTED: goto readerror;
    }
    for(n = 0; n < g.gl_pathc; ++n) {
      struct stat sb;
      char *path = g.gl_pathv[n];
      size_t l = strlen(path);

      /* skip already-compressed files */
      if(l >= 3 && !strcmp(path + l - 3, ".gz"))
	continue;
      if(lstat(path, &sb) < 0)
	continue;
      /* only care about regular files more than a day old,
       * i.e. better not comress a file we might re-open soon */
      if(S_ISREG(sb.st_mode)
	 && sb.st_mtime < now - ld_day) {
	pid_t pid, r;
	int status;

	/* try to compress the file */
	switch(pid = fork()) {
	case 0:
	  /* XXX should support more than just gzip */
	  execlp("gzip", "gzip", "-9f", path, (char *)0);
	  _exit(-1);
	case -1:
	  errore("fork");
	  break;
	default:
	  do {
	    r = waitpid(pid, &status, 0);
	  } while(r == -1 && errno == EINTR);
	  if(r == -1)
	    errore("error waiting for gzip");
	  else if(status)
	    error("wait status from gzip -9f %s: %#x",
		  path, (unsigned)status);
	}
      }
    }
    globfree(&g);
  }
  free(pattern);
  return;
nospace:
  error("glob ran out of memory");
  goto quit;
readerror:
  error("glob detected a read error");
quit:
  free(pattern);
  globfree(&g);
  return;
}

char *ld_globtime(const char *pattern) {
  char *s = 0;
  size_t n = 0, size = 0;
  int c;

  do {
    c = *pattern++;
    switch(c) {
    case '\\':
    case '[':
    case '?':
    case '*':
      if(n >= size)
	s = xrealloc(s, size = size ? size * 2 : 128);
      s[n++] = '\\';
      break;
    case '%':
      c = *pattern++;
      switch(c) {
      case '%':				/* literal % */
	break;
      case 't':				/* tab */
	c = '\t';
	break;
      case 'n':				/* newline */
	c = '\n';
	break;
      case 'a': case 'A':		/* weekday names */
      case 'b': case 'B': case 'h':	/* month names */
      case 'C':				/* century */
      case 'd':				/* day of month */
      case 'e':				/* day of month */
      case 'g': case 'G':		/* year */
      case 'H': case 'I':		/* hour */
      case 'k': case 'l':		/* hour */
      case 'j':				/* day of year */
      case 'm':				/* month */
      case 'M':				/* minute */
      case 'p': case 'P':		/* am/pm */
      case 'r':				/* time + am/pm */
      case 'R':				/* time */
      case 's':				/* time_t */
      case 'S':				/* second */
      case 'T':				/* time */
      case 'u': case 'w':		/* day of week */
      case 'U': case 'V': case 'W':	/* week number */
      case 'y': case 'Y':		/* year */
      case 'Z':				/* timezone */
	c = '*';
	break;
      default:
	free(s);
	return 0;
      }
      break;
    }
    if(n >= size)
      s = xrealloc(s, size = size ? size * 2 : 128);
    s[n++] = c;
  } while(c);
  return s;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
