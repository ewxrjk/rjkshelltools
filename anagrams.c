/* (c) 1997, 2002 Richard Kettlewell.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>

#include "utils.h"
#include "wordlist.h"

#define HASHSIZE 1024

static struct option const long_options[] =
{
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'V' },
  { "wordlist", required_argument, 0, 'w' },
  { "word", required_argument, 0, 'e' },
  { 0, 0, 0, 0}
};

/* write a usage message to FP and exit with the specified status */
static void __attribute__((noreturn)) usage(FILE *fp, int exit_status) {
  if(fputs(
"Usage:\n"
"  anagrams [options] [--] [words ...]\n"
"\n"
"Options:\n"
"  -w PATH, --wordlist PATH              Path to word list\n"
"  -e WORD, --word WORD                  Add a word to the word list\n"
"  -h, --help                            Usage message\n"
"  -V, --version                         Version number\n"
, fp) < 0)
    fatale("output error");
  exit(exit_status);
}

struct word {
    struct word *next;			/* next word */
    struct word *nextsamehash;		/* next word with same hash */
    char *word;				/* the word itself */
    char *sorted;			/* sorted version of the word */
} *words, **wordend = &words;

static struct word *hashtable[HASHSIZE]; /* hash table for deduping */

static void check(const char *prefix, const char *s, const struct word *w);
static void anagrams(const char *word);
static void sortword(char *word);
static int comparator(const void *a, const void *b);
static void addword(char *word);
static char *stripnl(char *line);
static void import_wordlist(const char *path);

int main(int argc, char *argv[]) {
    char *line;
    int i;
    int n;
    int imported_some_words = 0;

    setprogname(argv[0]);
    
    while((n = getopt_long(argc, argv, 
			   "hVw:e:",
			   long_options, (int *)0)) >= 0) {
	switch(n) {
	case 'V':
	    printf("anagrams %s\n", VERSION);
	    return 0;
	    
	case 'h':
	    usage(stdout, 0);
	    return 0;

	case 'w':
	    import_wordlist(optarg);
	    imported_some_words = 1;
	    break;

	case 'e':
	    addword(optarg);
	    break;
	    
	default:
	    usage(stderr, 1);
	}
    }

    if(!imported_some_words)
	import_wordlist(DEFAULT_WORDLIST);
    
    if(optind < argc) {
	/* check words listed on command line */
	for(i = optind; i < argc; i++) {
	    puts(argv[i]);
	    sortword(argv[i]);
	    anagrams(argv[i]);
	}
    } else {
	int tty = isatty(0);
	
	/* read words from stdin and check them */
	if(tty && printf("Ready\n") < 0)
	    fatale("error writing to stdout");
	while((line = get_line(stdin))) {
	    stripnl(line);
	    sortword(line);
	    anagrams(line);
	    if(tty && printf("Ready\n") < 0)
		fatale("error writing to stdout");
	}
	if(ferror(stdin))
	    fatale("error reading from stdin");
    }
    if(fclose(stdout))
	fatale("error closing stdout");
    return 0;
}

/* read in all the words from PATH */
static void import_wordlist(const char *path) {
    FILE *fp;
    char *line;
    
    /* read in the word list */
    if(!(fp = fopen(path, "r")))
	fatale("error opening %s", path);
    while((line = get_line(fp))) {
	addword(stripnl(line));
	free(line);
    }
    if(ferror(fp))
	fatale("error reading from %s", path);
    *wordend = NULL;
    fclose(fp);
}

/* remove the final newline of LINE */
static char *stripnl(char *line) {
    char *eol = strchr(line, '\n');
    
    if(eol)
	*eol = 0;
    return line;
}
    
/* sort a word in place */

static void sortword(char *word) {
    qsort(word, strlen(word), 1, comparator);
}

/* add a word to the word list */
static void addword(char *word) {
    char *p;
    struct word *w;
    int h;
    
    for(p = word; *p; p++) {
	*p = tolower(*p);
    }
    /* check for duplicates */
    h = hash(word) % HASHSIZE;
    for(w = hashtable[h]; w && strcmp(word, w->word); w = w->nextsamehash)
	;
    if(w)
	return;
    w = xmalloc(sizeof (struct word));
    w->word = xstrdup(word);
    sortword(word);
    w->sorted = xstrdup(word);
    w->nextsamehash = hashtable[h];
    hashtable[h] = w;
    *wordend = w;
    wordend = &w->next;
}    

static int comparator(const void *a, const void *b) {
    return *(unsigned char *)a - *(unsigned char *)b;
}

/* return nonzero if the candidate W->WORD could not be part of an
 * anagram of WORD; i.e. if it contains more copies of some character
 * than appear in WORD. */
static int strippable(const struct word *w, const char *word) {
    const char *t = w->sorted;
    const char *u = word;

    /* both words are sorted, so we can just do a linear scan */
    while(*t && *u) {
	/* skip any characters in WORD that are not in the candidate.
	 * We don't mind these existing because they might be matched
	 * by some other candidate. */
	while(*t != *u && *u) {
	    ++u;
	}
	/* if we reached the end of WORD without finding a match for
	 * *u then the character from the candidate at *t must fail to
	 * match any in the word; so this candidate is strippable. */
	if(!*u) {
	    return 1;
	}
	/* ergo *u = *v */
	++t;
	++u;
    }
    return 0;
}

/* list all the anagrams of WORD */
static void anagrams(const char *word) {
    struct word *words_sublist, **wp = &words_sublist;
    struct word *w;

    /* first construct a subset list of words, which contains only
     * words that can actually be used.  See above for details. */
    for(w = words; w; w = w->next) {
	if(!strippable(w, word)) {
	    struct word *nw = xmalloc(sizeof (struct word));
	    *nw = *w;
	    *wp = nw;
	    wp = &nw->next;
	}
    }
    *wp = 0;
    check("", word, words_sublist);
    w = words_sublist;
    while(w) {
	struct word *o = w;
	w = w->next;
	free(o);
    }
}

/* check word <s> in word list <w> ... <prefix> has the words that
   have been picked out so far.  <s> is sorted.  We print the results
   to stdout. */

static void check(const char *prefix, const char *s, const struct word *w) {
    const char *t, *u;
    int n;
    char *remainder = xmalloc(strlen(s) + 1);

    for(; w; w = w->next) {
	/* we want to see if the characters of <w->sorted> all appear
	   in <s>.  We know that both are sorted (as arrays of
	   unsigned char).  Therefore all equal letters must be
	   grouped together. */
	u = s;
	for(t = w->sorted; *t && *u; t++) {
	    /* skip over any characters in S that aren't in the
	     * candidate word */
	    while(*t != *u && *u) {
		u++;
	    }
	    /* if we've reached the end of S then we must have found a
	     * character in the candidate word that isn't in S */
	    if(!*u) {
		break;
	    }
	    u++;
	}
	/* did we reach the end matching every character? */
	if(!*t) {
	    char *newprefix;
	    
	    u = s;
	    n = 0;
	    for(t = w->sorted; *t && *u; t++) {
		while(*t != *u && *u) {
		    remainder[n++] = *u++;
		}
		u++;
	    }
	    while(*u) {
		remainder[n++] = *u++;
	    }
	    remainder[n] = 0;
	    newprefix = xmalloc(strlen(prefix) + strlen(w->word) + 2);
	    sprintf(newprefix, "%s %s", prefix, w->word);
	    if(n == 0) {
		if(puts(newprefix) < 0)
		    fatale("error writing to stdout");
	    } else {
		check(newprefix, remainder, w);
	    }
	    free(newprefix);
	}
    }
    free(remainder);
}

/*
Local Variables:
c-basic-offset:4
comment-column:40
End:
*/
