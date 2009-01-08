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

executable() {
  echo "$1" >> "$tested"
}

test_status=0

PATH=`pwd`:$PATH

# create a work directory
tested=`pwd`/tested
tmp=`pwd`/$$
mkdir $tmp
trap "cd / && rm -rf $tmp" EXIT INT HUP TERM
cd $tmp
