/* $Header: /cvs/rjkshelltools/inplace.c,v 1.7 2002-11-21 14:58:52 richard Exp $ */

/*

  inplace - modify files in-place via any filter
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


  Send bug reports to richard+inplace@sfere.greenend.org.uk
  
*/

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glob.h>
#include <locale.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <getopt.h>

#include "utils.h"

/* possible sources of lists of filenames */

enum input_type {
  type_filename,			/* a single filename */
  type_pattern,				/* a glob pattern to expand */
  type_file,				/* a file to read rom */
};

/* one element of the list of sources of lists of filenames */

struct input {
  enum input_type type;			/* which kind of source */
  const char *value;			/* filename or pattern */
};

/* record of a running command */

struct subcommand {
  pid_t pid;				/* process ID */
  int file;				/* associated file number */
};

int filename_terminator = '\n';		/* terminator in files */
const char *backup_suffix;		/* append to backup files */
int continue_after_error;		/* process as many as possible */
int read_from_stdin = 1;		/* read filenames from stdin? */
int max_subcommands = 1;		/* how parallel? */
int list_unprocessed;			/* dump unprocess to stdout? */
int glob_flags = GLOB_ERR|GLOB_NOSORT;	/* for glob(3) to implement -g */
int failed;				/* number of files that failed */
int running;				/* number of running subcommands */

/* list of sources of lists of filenames */

size_t number_of_inputs;
size_t input_slots;
struct input *inputs;

/* the eventual list of filenames */

size_t number_of_filenames;
size_t filename_slots;
const char **filenames;
int *results;

/* table of running subcommands */

static struct subcommand *subcommands;

/* add some kind of source of list of filenames */

static void add_input(enum input_type type,
		      const char *parameter) {
  if(number_of_inputs >= input_slots) {
    input_slots = input_slots ? 2 * input_slots : 128;
    inputs = xrealloc(inputs, sizeof (struct input) * input_slots);
  }
  inputs[number_of_inputs].type = type;
  inputs[number_of_inputs].value = parameter;
  ++number_of_inputs;
}

/* add one filename */

static void input_filename(const char *path) {
  if(number_of_filenames >= INT_MAX)
    fatal("too many filenames");
  if(number_of_filenames >= filename_slots) { 
    filename_slots = filename_slots ? 2 * filename_slots : 128;
    filenames = xrealloc(filenames, sizeof (char *) * filename_slots);
  }
  filenames[number_of_filenames] = path;
  ++number_of_filenames;
}

/* read filenames from an open file */

static void input_open_file(const char *name, FILE *fp) {
  char *buffer = 0, *top, *ptr;
  size_t bufsize = 0, n = 0;
  int c;

  while((c = getc(fp)) >= 0) {
    if(n >= bufsize) {
      bufsize = bufsize ? 2 * bufsize : 1024;
      buffer = xrealloc(buffer, bufsize);
    }
    buffer[n++] = c;
  }
  if(ferror(fp))
    fatale("error reading file %s", name);
  /* parse the filenames */
  top = buffer + n;
  ptr = buffer;
  while(ptr < top) {
    char *terminator = memchr(ptr, filename_terminator, top - ptr);

    if(!terminator)
      fatal("missing terminator in file %s", name);
    *terminator = 0;
    input_filename(ptr);
    ptr = terminator + 1;
  }
}

/* read filenames from a named file */

static void input_file(const char *name) {
  if(!strcmp(name, "-"))
    input_open_file("-", stdin);
  else {
    FILE *fp;

    if(!(fp = fopen(name, "r")))
      fatale("error opening %s", name);
    input_open_file(name, fp);
    fclose(fp);
  }
}

/* error callback for glob */

static int __attribute__((noreturn)) globerror(const char *path, int errno_value) {
  fatal("error scanning files: %s: %s", path, strerror(errno_value));
}

/* generate filenames from a pattern */

static void input_pattern(const char *pattern) {
  glob_t g;
  size_t n;

  switch(glob(pattern, glob_flags, globerror, &g)) {
  case GLOB_NOSPACE:
    fatal("error calling glob: %s", strerror(ENOMEM));
#ifdef GLOB_ABORTED
  case GLOB_ABORTED:
    fatal("read error in glob");
#endif
#ifdef GLOB_ABEND
    fatal("glob failed");
#endif
#ifdef GLOB_NOMATCH
  case GLOB_NOMATCH:
    return;
#endif
  }
  for(n = 0; n < g.gl_pathc; ++n)
    input_filename(g.gl_pathv[n]);
}

/* compute the temporary filename used for filter output.  Needs to
 * know the subcommand's PID. */

static char *output_filename(int file, pid_t pid,
			     char buffer[], size_t bufsize) {
  int n;

  if((n = snprintf(buffer, bufsize, "%s.inplace-tmp-%lx",
		   filenames[file], (unsigned long)pid)) < 0
     || (size_t)n >= bufsize)
    fatal("filename too long");
  return buffer;
}

/* called whenever a subcommand terminates */

static void subcommand_finished(pid_t pid, int wstat) {
  int n;
  int file;
  const char *output_name;
  char buffer[PATH_MAX];

  /* find the subcommand */
  for(n = 0; n < max_subcommands && subcommands[n].pid != pid; ++n)
    ;
  /* ignore irrelevant processes */
  if(n >= max_subcommands) {
    error("unknown subprocess %lu terminated with wstat %x",
	  (unsigned long)pid, (unsigned)wstat);
    return;
  }
  file = subcommands[n].file;
  output_name = output_filename(file, pid, buffer, sizeof buffer);
  if(!wstat) {
    /* implement -b option */
    if(backup_suffix) {
      char backup_name[PATH_MAX];
      int s;

      if((s = snprintf(backup_name, sizeof backup_name,
		       "%s%s", filenames[file], backup_suffix)) < 0
	 || (size_t)s >= sizeof backup_name) {
	error("backup filename too long");
	wstat = 1;
      } else if(link(filenames[file], backup_name) < 0) {
	errore("error linking %s to %s", filenames[file], backup_name);
	wstat = 1;
      }
    }
    if(!wstat && rename(output_name, filenames[file]) < 0) {
      errore("error renaming %s to %s", output_name, filenames[file]);
      wstat = 1;
    }
  }
  /* clean up the temporary file - shouldn't be necessary if
   * everything succeeded */
  if(wstat && remove(output_name) < 0 && errno != ENOENT)
    errore("error removing %s", output_name);
  /* record success/failure */
  results[file] = wstat;
  if(wstat)
    ++failed;
  /* report signals, since the subcommand may lack the opportunity to
   * do so */
  if(WIFSIGNALED(wstat))
    error("filter for %s exited on signal %d (%s)%s",
	  filenames[file],
	  WTERMSIG(wstat),
	  strsignal(WTERMSIG(wstat)),
	  WCOREDUMP(wstat) ? " (core dumped)" : "");
  /* account for the terminated process */
  --running;
  subcommands[n].pid = -1;
}

static int usage(FILE *fp) {
  return fprintf(fp,
"Usage:\n"
"\n"
"  inplace OPTIONS -- COMMAND ...\n"
"\n"
"Options:\n"
"\n"
"  -0, --null                     Filenames terminate with 0, not \\n\n"
"  -b SUFFIX, --backup=SUFFIX     Backup files, append SUFFIX to name\n"
"  -c, --continue                 Continue after errors\n"
"  -f FILENAME, --file=FILENAME   Process FILENAME\n"
"  -g, --extended-glob            Extra syntax for --pattern\n"
"  -i FILE, --input=FILE          Read filenames to process from FILE\n"
"  -j MAX, --jobs=MAX             Up to MAX concurrent jobs (default 1)\n"
"  -l, --list                     List unprocessed files to stdout\n"
"  -p PATTERN, --pattern=PATTERN  Process files matching PATTERN\n"
"  --version                      Display version number\n");
}

#define OPT_HELP (UCHAR_MAX + 1)
#define OPT_VERSION (UCHAR_MAX + 2)

static const struct option options[] = {
  { "null", 0,0, '0' },
  { "backup", 1, 0, 'b' },
  { "continue", 0, 0, 'c' },
  { "file", 1, 0, 'f' },
  { "extended-glob", 0, 0, 'g' },
  { "input", 1, 0, 'i' },
  { "jobs", 1, 0, 'j' },
  { "list", 0, 0, 'l' },
  { "pattern", 1, 0, 'p' },
  { "help", 0, 0, OPT_HELP },
  { "version", 0, 0, OPT_VERSION },
  { 0, 0, 0, 0 },
};

int main(int argc, char **argv) {
  int n;
  long l;
  char *e;
  int file;

  /* pick up program name from argv */
  setprogname(argv[0]);

  /* error messages in native language */
  errno = 0;
  if(!setlocale(LC_MESSAGES, ""))
    fatale("error calling setlocale");
  /* process argv */
  while((n = getopt_long(argc, argv, "0clgf:p:i:b:j:", options, 0)) >= 0) {
    switch(n) {
    case '0':
      filename_terminator = 0;
      break;
    case 'b':
      if(backup_suffix)
	fatal("multiple '-b' options make no sense");
      backup_suffix = optarg;
      break;
    case 'c':
      continue_after_error = 1;
      break;
    case 'f':
      read_from_stdin = 0;
      add_input(type_filename, optarg);
      break;
    case 'g':
#if defined GLOB_BRACE && defined GLOB_TILDE
      glob_flags |= GLOB_BRACE|GLOB_TILDE;
#else
      fatal("-g is not supported on this system");
#endif
      break;
    case 'i':
      read_from_stdin = 0;
      add_input(type_file, optarg);
      break;
    case 'j':
      errno = 0;
      l = strtol(optarg, &e, 10);
      if(errno)
	fatale("invalid argument to '-j'");
      if(e == optarg || *e || l <= 0 || l > INT_MAX)
	fatal("invalid argument to '-j'");
      max_subcommands = l;
      break;
    case 'l':
      list_unprocessed = 1;
      break;
    case 'p':
      read_from_stdin = 0;
      add_input(type_pattern, optarg);
      break;
    case OPT_HELP:
      if(usage(stdout) < 0
	 || fclose(stdout) < 0)
	fatale("error writing to  stdout");
      return 0;
    case OPT_VERSION:
      if(printf("%s\n", VERSION) < 0
	 || fclose(stdout) < 0)
	fatale("error writing to  stdout");
      return 0;
    default:
      exit(-1);
    }
  }
  /* if no other filename source specified, use stdin */
  if(read_from_stdin)
    input_open_file("-", stdin);
  /* read filenames from specified sources */
  for(n = 0; (size_t)n < number_of_inputs; ++n) {
    switch(inputs[n].type) {
    case type_filename:
      input_filename(inputs[n].value);
      break;
    case type_pattern:
      input_pattern(inputs[n].value);
      break;
    case type_file:
      input_file(inputs[n].value);
      break;
    }
  }
  /* allocate memory for tracking subcommands */
  subcommands = xmalloc(max_subcommands * sizeof (struct subcommand));
  for(n = 0; n < max_subcommands; ++n) {
    subcommands[n].pid = -1;
    subcommands[n].file = -1;
  }
  results = xmalloc(number_of_filenames * sizeof (int));
  for(n = 0; (size_t)n < number_of_filenames; ++n)
    results[n] = -1;
  file = 0;				/* next file to process */
  running = 0;				/* number of running subcommands */
  while(running > 0
	|| ((size_t)file < number_of_filenames
	    && (failed == 0 || continue_after_error))) {
    /* run additional subcommands if necessary/possible */
    while((size_t)file < number_of_filenames
	  && running < max_subcommands
	  && (failed == 0 || continue_after_error)) {
      for(n = 0; n < max_subcommands && subcommands[n].pid != -1; ++n)
	;
      subcommands[n].file = file;
      switch(subcommands[n].pid = fork()) {
	struct stat sb;
	const char *output_name;
	char buffer[PATH_MAX];
	int fd;
	
      case 0:
	/* set up input */
	fd = open_e(filenames[file], O_RDONLY, 0);
	dup2_e(fd, 0);
	close_e(fd);
	/* set up output */
	output_name = output_filename(file, getpid(), buffer, sizeof buffer);
	fd = open_e(output_name,
		    O_WRONLY|O_CREAT|O_EXCL,
		    0600);
	dup2_e(fd, 1);
	close_e(fd);
	/* fix up permissions */
	if(fstat(0, &sb) < 0)
	  fatale("error calling stat for %s", filenames[file]);
	if(fchown(1, sb.st_uid, sb.st_gid) < 0)
	  fatale("error calling fchown on %s", output_name);
	if(fchmod(1, sb.st_mode) < 0)
	  fatale("error calling fchmod on %s", output_name);
	/* execute the program */
	if(execvp(argv[optind], argv + optind) < 0)
	  fatale("error calling execvp");
	fatal("execvp unexpectedly returned");
	
      case -1:
	errore("error calling fork");
	results[file] = 1;
	++file;
	++failed;
	break;
	
      default:
	++file;
	++running;
	break;
      }
    }
    /* if there are subcommands running, wait until one finishes */
    if(running) {
      int w;
      pid_t pid;

      pid = waitpid_e(-1, &w, 0);
      if(pid > 0) {
	subcommand_finished(pid, w);
	/* pick up any other subcommands that finished, but don't
	 * block */
	while(running > 0 && (pid = waitpid_e(-1, &w, WNOHANG)) > 0)
	  subcommand_finished(pid, w);
      }
    }
  }
  /* if -l was specified, dump the list of unprocessed files */
  if(list_unprocessed) {
    for(n = 0; (size_t)n < number_of_filenames; ++n)
      if(results[n] != 0)
	if(printf("%s%c", filenames[n], filename_terminator) < 0)
	  fatale("error writing to stdout");
  }
  /* flush stdout */
  if(fclose(stdout) < 0)
    fatale("error closing stdout");
  /* signal results to caller */
  return failed && !list_unprocessed ? -1 : 0;
}

/*
Local Variables:
mode:c
c-basic-offset:2
comment-column:40
End:
*/
