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

#ifndef LOGDAEMON_H
#define LOGDAEMON_H

struct logfile {
  struct logfile *next; /* next logfile */
  int refs;             /* reference count */
  char *pattern;        /* filename pattern */
  char *path;           /* open path (or 0) */
  int fd;               /* output file descriptor or -1 */
  long date;            /* date of open version */
  int rotate;           /* number of days to keep */
  int compress;         /* whether to compress old copies */
  int usegmt;           /* use GMT in names */
  char *buffer;         /* buffer for saved data */
  size_t bufsize;       /* buffer size */
};

struct syslogfile {
  struct syslogfile *next; /* next logfile */
  int pri;                 /* priority */
  int fac;                 /* facility */
  char *buffer;            /* line buffer */
  size_t bufsize;          /* buffer size */
  size_t bytes;            /* bytes in buffer */
};

struct input {
  struct input *next;       /* next input */
  int fd;                   /* file descriptor */
  struct timeval suspended; /* suspended due to errors */
  void (*input_callback)(struct input *, struct timeval); /* input callback */
  void (*daily_callback)(struct input *, struct timeval); /* daily callback */
  void *log; /* logfile to write to */
};

/* create a new logfile object.  Initialize the pattern field with a
 * pointer to a copy of PATTERN.  rotate and compress are 0 by
 * default, usegmt is 1 by default.
 *
 * If a logfile object with the same pattern already exists, that is
 * returned instead.
 */
struct logfile *ld_new_logfile(const char *pattern);

/* delete a logfile object.  Reference counting is used to ensure that
 * logfiles that were returned more than once by a call to
 * ld_new_logfile aren't deleted unexpectedly. */
void ld_delete_logfile(struct logfile *l);

/* create a new syslog logfile object.  PRIORITY is a level.priority
 * string as found in syslog.conf.
 *
 * If the priority is invalid, then 0 is returned.
 */
struct syslogfile *ld_new_syslogfile(const char *priority);

/* delete a syslog logfile object. */
void ld_delete_syslogfile(struct syslogfile *l);

/* create a new input object.  Initialize the fd field from FD and the
 * log field from L.  The callbacks are set to write log messages
 * received on FD to log L.
 */
struct input *ld_new_input(int fd, void *l);

/* delete an input object */
void ld_delete_input(struct input *i);

/* mark an input as suspended.  No attempt to read from it will be
 * made for the next few seconds.  If the input is already suspended,
 * this has no effect. */
void ld_suspend_input(struct input *i);

/* de-suspend an input.  When an input is de-suspended, its input
 * callback is invoked.  If it is not suspended, this has no
 * effect. */
void ld_resume_input(struct input *i);

/* open the output file for logfile L corresponding to time NOW.
 * Returns 0 on success and -1 on error. */

int ld_open_logfile(struct logfile *l, struct timeval now);

/* close any open output file for logfile L.  If no file is open, this
 * has no effect. */
void ld_close_logfile(struct logfile *l);

/* return the next time, after NOW, that rotation, compression, etc,
 * should be started. */
struct timeval ld_next_daily(struct timeval now);

/* wait for and process events.  When there are no more inputs,
 * returns 0 (so set some up before calling!).  If an error occurs
 * that can't be safely dealt with, returns -1.
 *
 * when a non-suspended input's file descriptor is readable, its input
 * callback is called.  Also, at or shortly after the times returned
 * by ld_next_daily(), the daily callbacks are called (even for
 * suspended inputs). */
int ld_loop(void);

/* the default input callback.
 *
 * First, it opens the output file for the input's logfile at the
 * current time.  If this fails, suspends the input and returns.
 *
 * Next, it tries to flush any buffered data.  If it can't flush all
 * the buffered data, it suspends the input and returns.
 *
 * After this it reads some data and tries to write it to the output
 * file.  If the read fails, or EOF is detected, the input is deleted.
 * If some or all of it cannot be written, it is buffered and the
 * input is suspended.
 */
void ld_input_callback(struct input *i, struct timeval now);

/* the default daily callback.  Everything here happens in terms of
 * the logfile for input I, rather than the input itself.
 *
 * This only handles compression and rotation, and so does nothing if
 * both of these are disabled.
 *
 * First the output file is closed, if open.  Then the pattern is
 * converted to a glob pattern by translating any % expansions in it
 * into asterisks or other characters as appropriate.  Some
 * conversions, including at least %c, %D, %x and %X are not supported
 * and if the pattern includes one of these, the callback returns
 * immediately.  (The problem is that they do or may contain "/"
 * symbols, which results in the glob pattern being wrong for the set
 * of filenames generated.)
 *
 * If the rotate field is nonzero, then it searches for regular files
 * that match the pattern and are strictly older than that many days.
 * Compressed files are included if compression is in force.  Such
 * files are deleted, along with their containing directories if
 * empty.  Measures are taken to avoid deleting the wrong directories,
 * see the source for detail: when the stablize they will be more
 * fully documented here.
 *
 * If the compress field is nonzero then it looks for uncompressed
 * regular files that match the pattern and are strictly older than a
 * day.  For these files, gzip is invoked to compress them.
 */
void ld_daily_callback(struct input *i, struct timeval now);

/* alternative input callback for syslog support.  Data is read from
 * the input into a buffer.  Whenever a full line has been read, that
 * line is logged via syslog(3).  If there is an incomplete line at
 * the end of the input (i.e. missing its final newline) then that
 * line is logged too.
 *
 * The caller is responsible for calling openlog() if desired. */
void ld_syslog_callback(struct input *i, struct timeval now);

/* create a string containing the glob-ified version of a strftime
 * pattern.  Returns 0 on error. */
char *ld_globtime(const char *pattern);

/* linked list of logfiles */
extern struct logfile *ld_logfiles;

/* linked list of syslog logfiles */
extern struct syslogfile *ld_syslogfiles;

/* linked list of inputs */
extern struct input *ld_inputs;

/* number of seconds between rotations (usually 86400, i.e. one day) */
extern long ld_day;

/* compare timevals */
int tvcmp(const struct timeval *a, const struct timeval *b);

/* subtract timevals */
struct timeval tvsub(const struct timeval *a, const struct timeval *b);

#endif /* LOGDAEMON_H */

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
