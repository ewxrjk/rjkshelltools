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

#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

void setpriv(const char *user, const char *group, const char *root) {
  uid_t uid;
  gid_t gid;
  struct passwd *pw;
  struct group *gr;

  /* look up the user */
  if(!(pw = getpwnam(user)))
    fatal("no such user as \"%s\"", user);
  uid = pw->pw_uid;

  /* look up the group */
  if(group) {
    if(!(gr = getgrnam(group)))
      fatal("no such group as \"%s\"", group);
    gid = gr->gr_gid;
  } else
    gid = pw->pw_gid;

  /* set the supplementary group list */
  if(initgroups(user, gid) < 0)
    fatale("error calling initgroups");

  /* chroot, if we are doing so */
  if(root) {
    if(chroot(root) < 0)
      fatale("error calling chroot");
    if(chdir("/") < 0)
      fatale("error calling chdir");
  }

  /* set the group IDs */
  if(setgid(gid) < 0)
    fatale("error calling setgid");

  /* check it worked */
  if(getgid() != gid)
    fatal("real GID didn't change");
  if(getegid() != gid)
    fatal("effective GID didn't change");

  /* set the user IDs */
  if(setuid(uid) < 0)
    fatale("error calling setuid");

  /* check it worked */
  if(getuid() != uid)
    fatal("real UID didn't change");
  if(geteuid() != uid)
    fatal("effective UID didn't change");
}

/*
Local Variables:
c-basic-offset:2
comment-column:40
End:
*/
