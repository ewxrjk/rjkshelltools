# 
# This file is part of rjkshelltools
# Copyright (C) 2001-2003, 2014 Richard Kettlewell
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# report what we are testing
testing() {
  printf "checking that %s... " "$@"
  current_test="$1"
}

# report a success
ok() {
  echo OK
}

# report a failure
fail() {
  echo FAILED
  echo "[$current_test]" >> errors
  test_status=1
  if test $# != 0; then
    echo "$@" >> errors
  fi
}

report() {
  for path in "$@"; do
    echo "-$path-"
    cat $path
  done
  echo "--"
}

# call after all tests
finished() {
  if test $test_status = 1; then
    report errors
  fi
  exit $test_status
}

test_status=0

PATH=`pwd`:$PATH

# create a work directory
tmp=`pwd`/$$
mkdir $tmp
trap "cd / && rm -rf $tmp" EXIT INT HUP TERM
cd $tmp
