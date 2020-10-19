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
verificarlo-c --save-temps --verbose -c --function f1 a.c 2> a
did_instrument f1 a
did_not_instrument f2 a

echo "SUBTEST 2: Check that --include (include-list only mode) works with *"
cat > include.txt <<HERE
#this is a comment followed by a blank line

* f1

HERE
verificarlo-c --verbose -c --include-file include.txt a.c 2> a
did_instrument f1 a
did_not_instrument f2 a

echo "SUBTEST 3 : more complex include-list only"
cat > include.txt <<HERE
a f1
b f2
HERE
verificarlo-c --verbose -c --include-file include.txt a.c 2> a
verificarlo-c --verbose -c --include-file include.txt b.c 2> b
did_instrument f1 a
did_not_instrument f2 a
did_not_instrument f1 b
did_instrument f2 b

echo "SUBTEST 4 : exclude-list only"
cat > exclude.txt <<HERE
a f1
* f2
HERE
verificarlo-c --verbose -c --exclude-file exclude.txt a.c 2> a
verificarlo-c --verbose -c --exclude-file exclude.txt b.c 2> b
did_not_instrument f2 a
did_not_instrument f1 a
did_not_instrument f2 b
did_instrument f1 b

echo "SUBTEST 5 : include-list and exclude-list"
cat > include.txt <<HERE
b f2
HERE
cat > exclude.txt <<HERE
* f2
HERE
verificarlo-c --verbose -c --exclude-file exclude.txt --include-file include.txt a.c 2> a
verificarlo-c --verbose -c --exclude-file exclude.txt --include-file include.txt b.c 2> b
did_not_instrument f2 a
did_instrument f2 b
did_instrument f1 a
did_instrument f1 b

echo "SUBTEST 6 : --function and --exclude / --include are not compatible"
if verificarlo-c --verbose -c --exclude-file exclude.txt --function f1 a.c ; then
    echo "THIS SHOULD FAIL"
    exit 1
else
    echo "ok"
fi

echo "SUBTEST 7: Check that --include-file works with any module starting with dir"
cat > include.txt <<HERE
dir* *
HERE
verificarlo-c --verbose -c --include-file include.txt a.c 2> a
verificarlo-c --verbose -c --include-file include.txt b.c 2> b
verificarlo-c --verbose -c --include-file include.txt dir1/a.c 2> dir1_a
verificarlo-c --verbose -c --include-file include.txt dir1/b.c 2> dir1_b
verificarlo-c --verbose -c --include-file include.txt dir2/a.c 2> dir2_a
verificarlo-c --verbose -c --include-file include.txt dir2/b.c 2> dir2_b

did_not_instrument f1 a
did_not_instrument f2 a
did_not_instrument g1 a
did_not_instrument g2 a

did_not_instrument f1 b
did_not_instrument f2 b
did_not_instrument g1 b
did_not_instrument g2 b

did_instrument f1 dir1_a
did_instrument f2 dir1_a
did_instrument g1 dir1_a
did_instrument g2 dir1_a

did_instrument f1 dir1_b
did_instrument f2 dir1_b
did_instrument g1 dir1_b
did_instrument g2 dir1_b

did_instrument f1 dir2_a
did_instrument f2 dir2_a
did_instrument g1 dir2_a
did_instrument g2 dir2_a

did_instrument f1 dir2_b
did_instrument f2 dir2_b
did_instrument g1 dir2_b
did_instrument g2 dir2_b

echo "SUBTEST 8: Check include-list and exclude-list "
cat > include.txt <<HERE
dir2 g*
HERE
cat > exclude.txt <<HERE
dir*/a *1
dir1/* g2
HERE

verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt a.c 2> a
verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt b.c 2> b
verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt dir1/a.c 2> dir1_a
verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt dir1/b.c 2> dir1_b
verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt dir2/a.c 2> dir2_a
verificarlo-c --verbose -c --include-file include.txt --exclude-file exclude.txt dir2/b.c 2> dir2_b

did_instrument f1 a
did_instrument f2 a
did_instrument g1 a
did_instrument g2 a

did_instrument f1 b
did_instrument f2 b
did_instrument g1 a
did_instrument g2 a

did_not_instrument f1 dir1_a
did_instrument f2 dir1_a
did_not_instrument g1 dir1_a
did_not_instrument g2 dir1_a

did_instrument f1 dir1_b
did_instrument f2 dir1_b
did_instrument g1 dir1_b
did_not_instrument g2 dir1_b

did_not_instrument f1 dir2_a
did_instrument f2 dir2_a
did_not_instrument g1 dir2_a
did_instrument g2 dir2_a

did_instrument f1 dir2_b
did_instrument f2 dir2_b
did_instrument g1 dir2_b
did_instrument g2 dir2_b

echo "SUBTEST 9: Check syntax for *"
cat > include.txt <<HERE
$PWD/a *
*/dir1/a.* f1
dir1/a.c f2
dir1/a g1
dir1/* g2
HERE
verificarlo-c --verbose -c --include-file include.txt a.c 2> a
verificarlo-c --verbose -c --include-file include.txt b.c 2> b
verificarlo-c --verbose -c --include-file include.txt dir1/a.c 2> dir1_a
verificarlo-c --verbose -c --include-file include.txt dir1/b.c 2> dir1_b
verificarlo-c --verbose -c --include-file include.txt dir2/a.c 2> dir2_a
verificarlo-c --verbose -c --include-file include.txt dir2/b.c 2> dir2_b

did_instrument f1 a
did_instrument f2 a
did_instrument g1 a
did_instrument g2 a

did_not_instrument f1 b
did_not_instrument f2 b
did_not_instrument g1 b
did_not_instrument g2 b

did_instrument f1 dir1_a
did_instrument f2 dir1_a
did_instrument g1 dir1_a
did_instrument g2 dir1_a

did_not_instrument f1 dir1_b
did_not_instrument f2 dir1_b
did_not_instrument g1 dir1_b
did_instrument g2 dir1_b

did_not_instrument f1 dir2_a
did_not_instrument f2 dir2_a
did_not_instrument g1 dir2_a
did_not_instrument g2 dir2_a

did_not_instrument f1 dir2_b
did_not_instrument f2 dir2_b
did_not_instrument g1 dir2_b
did_not_instrument g2 dir2_b
