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

#include <signal.h>
#include <sys/types.h>

#include "utils.h"

static volatile sig_atomic_t signals[NSIG + 1];
static void (*handlers[NSIG + 1])(int);
static struct sigaction oldsa[NSIG + 1];
static sigset_t handled, unblocked;
static int initialized;

static void handler(int sig) {
  signals[sig] = 1;
}

void handle_signal(int sig, void (*function)(int)) {
  handlers[sig] = function;
}

void init_signals(void) {
  if(!initialized) {
    int n;
    struct sigaction sa;
    
    sigemptyset(&handled);
    for(n = 1; n <= NSIG; ++n)
      if(handlers[n])
	sigaddset(&handled, n);
    sigprocmask_e(SIG_BLOCK, &handled, &unblocked);
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    for(n = 1; n <= NSIG; ++n)
      if(handlers[n])
	sigaction_e(n, &sa, &oldsa[n]);
    initialized = 1;
  }
}

void restore_signals(void) {
  if(initialized) {
    int n;
    
    for(n = 1; n <= NSIG; ++n)
      if(handlers[n])
	sigaction(n, &oldsa[n], 0);
    sigprocmask_e(SIG_SETMASK, &unblocked, 0);
    initialized = 0;
  }
}

void signal_loop(void) {
  int n;

  init_signals();
  for(;;) {
    sigsuspend(&unblocked);
    for(n = 1; n <= NSIG; ++n) {
      if(signals[n]) {
	signals[n] = 0;
	(*handlers[n])(n);
      }
    }
  }
}
