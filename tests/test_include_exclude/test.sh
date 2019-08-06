#!/bin/sh

did_not_instrument() {
  if grep "In Function: $1" $2 > /dev/null ;
  then
    echo "SHOULD NOT HAVE instrumented $1 in $2"
    exit 1
  else
    echo "not instrumented $1 in $2"
  fi
}

did_instrument() {
  if grep "In Function: $1" $2 > /dev/null ;
  then
    echo "instrumented $1 in $2"
  else
    echo "SHOULD HAVE instrumented $1 in $2"
    exit 1
  fi
}

echo "SUBTEST 1: Check that --function works"
verificarlo --verbose -c --function f1 a.c 2> a
did_instrument f1 a
did_not_instrument f2 a

echo "SUBTEST 2: Check that --include (white-list only mode) works with *"
cat > include.txt <<HERE
#this is a comment followed by a blank line

* f1

HERE
verificarlo --verbose -c --include-file include.txt a.c 2> a
did_instrument f1 a
did_not_instrument f2 a

echo "SUBTEST 3 : more complex white-list only"
cat > include.txt <<HERE
a f1
b f2
HERE
verificarlo --verbose -c --include-file include.txt a.c 2> a
verificarlo --verbose -c --include-file include.txt b.c 2> b
did_instrument f1 a
did_not_instrument f2 a
did_not_instrument f1 b
did_instrument f2 b

echo "SUBTEST 4 : black-list only"
cat > exclude.txt <<HERE
a f1
* f2
HERE
verificarlo --verbose -c --exclude-file exclude.txt a.c 2> a
verificarlo --verbose -c --exclude-file exclude.txt b.c 2> b
did_not_instrument f2 a
did_not_instrument f1 a
did_not_instrument f2 b
did_instrument f1 b

echo "SUBTEST 5 : white-list and black-list"
cat > include.txt <<HERE
b f2
HERE
cat > exclude.txt <<HERE
* f2
HERE
verificarlo --verbose -c --exclude-file exclude.txt --include-file include.txt a.c 2> a
verificarlo --verbose -c --exclude-file exclude.txt --include-file include.txt b.c 2> b
did_not_instrument f2 a
did_instrument f2 b
did_instrument f1 a
did_instrument f1 b

echo "SUBTEST 6 : --function and --exclude / --include are not compatible"
if verificarlo --verbose -c --exclude-file exclude.txt --function f1 a.c ; then
  echo "THIS SHOULD FAIL"
  exit 1
else
  echo "ok"
fi
