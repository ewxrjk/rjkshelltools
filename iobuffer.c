/*
   iobuffer - pipeline component for buffering IO

   Copyright (C) 2001 Richard Kettlewell

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
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "uio.h"
#include "utils.h"

static char *buffer;			/* base of buffer */
static size_t offset;			/* offset of start of text */
static size_t total_bytes;		/* total bytes in buffer */

static size_t readmin = 32768;		/* minimum read */
static size_t writemin = 32768;		/* minimum write */
static size_t buffer_size = 1048576;	/* maximum data to hold in memory */

static int seen_eof;			/* true if we've seen read eof */

static int stdinflags, stdoutflags;	/* saved F_GETFL flags */

/* Option flags and variables */
static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "read-min", required_argument, 0, 'r' },
  { "write-min", required_argument, 0, 'w' },
  { "buffer", required_argument, 0, 'b' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */

static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  iobuffer [options]\n"
"\n"
"Options:\n"
"  -r N, --read-min N                Read at least N bytes per call\n"
"  -w N, --write-min N               Write at least N bytes per call\n"
"  -b N, --buffer N                  Specify buffer size\n"
"  -h, --help                        Usage message\n"
"  -V, --version                     Version number\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

/* fix up stdin/stdout flags */
static void restore_flags(void) {
  exiter = _exit;			/* don't recursively call exit() */
  fcntl_e(0, F_SETFL, stdinflags);
  fcntl_e(1, F_SETFL, stdoutflags);
}

/* fix up stdin/stdout flags */
static void fatal_signal_restore_flags(int signo) {
  sigset_t ss;

  restore_flags();
  signal(signo, SIG_DFL);
  if(kill(getpid(), signo) < 0)
    fatale("error calling kill");
  sigemptyset(&ss);
  sigaddset(&ss, signo);
  sigprocmask_e(SIG_UNBLOCK, &ss, 0);
}

/* restore flags if we die on a signal */
static void fix_fatal_signal(int n) {
  struct sigaction sa;

  if(n != SIGKILL && n != SIGSTOP) {
    sigaction_e(n, 0, &sa);
    if(sa.sa_handler != SIG_IGN) {
      sa.sa_handler = fatal_signal_restore_flags;
      sa.sa_flags = 0;
      sigfillset(&sa.sa_mask);
      sigaction_e(n, &sa, 0);
    }
  }
}

/* return true if we want to read */
static int want_to_read(void) {
  /* we want to read if there's at least readmin bytes available and
   * we've not seen eof */
  return !seen_eof && buffer_size - total_bytes >= readmin;
}

/* read some data */
static void do_read(void) {
  struct iovec vector[2];
  int n;
  size_t firstgap;
  int bytes_read;
  
  if(!want_to_read())
    return;
  n = 0;
  if(total_bytes == 0) {
    /* if the buffer is empty, use it all */
    offset = 0;
    vector[n].iov_base = buffer;
    vector[n].iov_len = buffer_size;
    ++n;
  } else {
    /* at least readmin bytes available; read whatever's coming */
    firstgap = (offset + total_bytes) % buffer_size; /* start of free space */
    if(firstgap < offset) {
      /* text wraps (or goes up to the end of the buffer); there is
       * only one gap to read into. */
      vector[n].iov_base = buffer + firstgap;
      vector[n].iov_len = offset - firstgap; 
      ++n;
   } else {
      /* there's a gap at the top of the buffer (and maybe one at the
       * bottom too) */
      vector[n].iov_base = buffer + firstgap;
      vector[n].iov_len = buffer_size - firstgap;
      ++n;
      if(offset > 0) {
	/* there's a gap at the bottom of the buffer */
	vector[n].iov_base = buffer;
	vector[n].iov_len = offset;
	++n;
      }
    }
  }
  bytes_read = readv(0, vector, n);
  if(bytes_read > 0)
    total_bytes += bytes_read;
  else if(!bytes_read)
    seen_eof = 1;
  else
    switch(errno) {
    case EINTR:
    case EAGAIN:
      break;
    default:
      fatale("error calling readv");
    }
}

/* return true if we want to write */
static int want_to_write(void) {
  /* we want to write either if we've seen eof and there's bytes left
   * to write, or if there's at least writemin bytes to write */
  return (seen_eof && total_bytes > 0) || total_bytes >= writemin;
}

/* write some data */
static void do_write(void) {
  struct iovec vector[2];
  int n = 0;
  size_t end;
  int bytes_written;

  if(!want_to_write())
    return;
  /* we might have one or two chunks to write, depending whether the
   * text wraps the end of the buffer */
  end = (offset + total_bytes) % buffer_size; /* end of text */
  if(end > offset) {
    /* text does not wrap; we have just one chunk to write */
    vector[n].iov_base = buffer + offset;
    vector[n].iov_len = total_bytes;
    ++n;
  } else {
    /* text wraps the buffer; we have two chunks to write */
    vector[n].iov_base = buffer + offset;
    vector[n].iov_len = buffer_size - offset;
    ++n;
    vector[n].iov_base = buffer;
    vector[n].iov_len = end;
    ++n;
  }
  bytes_written = writev(1, vector, n);
  if(bytes_written > 0)
    total_bytes -= bytes_written;
  else
    switch(errno) {
    case EINTR:
    case EAGAIN:
      break;
    default:
      fatale("error calling writev");
    }
}
 
int main(int argc, char **argv) {
  int n;
  long value;

  setprogname(argv[0]);

  while((n = getopt_long(argc, argv,
			 "r:w:b:hV",
			 long_options, (int *)0)) >= 0) {
    switch(n) {
    case 'V':
      printf("iobuffer %s\n", VERSION);
      return 0;

    case 'h':
      usage(stdout, 0);

    case 'r':
      if((value = atol(optarg)) <= 0)
	fatal("--read-min value must be positive");
      readmin = value;
      break;

    case 'w':
      if((value = atoi(optarg)) <= 0)
	fatal("--write-min value must be positive");
      writemin = value;
      break;

    case 'b':
      if((value = atoi(optarg)) <= 0)
	fatal("--buffer value must be positive");
      buffer_size = value;
      break;

    default:
      usage(stderr, 1);
    }
  }

  if(optind != argc)
    usage(stderr, 1);

  if(readmin > buffer_size)
    fatal("--read-min must be smaller than --buffer");
  if(writemin > buffer_size)
    fatal("--write-min must be smaller than --buffer");
  
  buffer = xmalloc(buffer_size);
  
  stdinflags = fcntl_e(0, F_GETFL, 0);
  stdoutflags = fcntl(1, F_GETFL, 0);
  /* exit() should restore the file flags */
  atexit(restore_flags);
  /* so should fatal signals.  We don't touch the ones that dump core, as the
   * usually indicate undefined behaviour, so anything we do might go wrong,
   * destroy evidence, etc.  XXX we should do similar fixups on the terminal
   * stop/continue signals too.  */
  fix_fatal_signal(SIGHUP);
  fix_fatal_signal(SIGINT);
  fix_fatal_signal(SIGPIPE);
  fix_fatal_signal(SIGALRM);
  fix_fatal_signal(SIGTERM);
  fix_fatal_signal(SIGUSR1);
  fix_fatal_signal(SIGUSR2);

  nonblock(0);
  nonblock(1);
  
  do {
    fd_set readable, writable;

    FD_ZERO(&readable);
    if(want_to_read())
      FD_SET(0, &readable);
    FD_ZERO(&writable);      
    if(want_to_write())
      FD_SET(1, &writable);
    if((n = select(2, &readable, &writable, 0, 0)) < 0) {
      if(errno == EINTR)
	continue;
      fatale("error calling select");
    }
    if(FD_ISSET(0, &readable))
      do_read();
    if(FD_ISSET(1, &writable))
      do_write();
    /* keep going while there are bytes left in the buffer, or in the
     * file */
  } while(!seen_eof || total_bytes);
  /* atexit callback will restore flags */
  return 0;
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
