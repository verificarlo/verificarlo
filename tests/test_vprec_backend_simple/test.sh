#!/bin/bash
set -xe

export VFC_BACKENDS_LOGGER=False

./compile.sh

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir -p $DIR
    cp -r compute_mpfr_rounding *.c *.py $DIR
    cd $DIR
}

test1() {
    new_env test1
    ## Denormal tests for range 2, precision 3
    cat >input.txt <<EOF
0x1.a53a8b6373154p-1 0x1.cfa1850291880p-4
-0x1.6312bf6a7f2f8p-1 -0x1.5b403af9711e8p-3
0x1.e4f85f0e582c0p-1 0x1.4b57c729ba5e0p-3
0x1.9d6aa71f9cb18p-1 0x1.9fdf01e9551c8p-1
0x1.8p-1 0x1.8p-1
0x1.f41e5c48bb0b8p-2 -0x1.2df365dba47ecp-1
EOF
    ../compare.sh OB 2 3 float x input.txt
}

test2() {
    new_env test2
    cat >input.txt <<EOF
0x1.a53a8b6373154p-1 0x1.cfa1850291880p-4
-0x1.6312bf6a7f2f8p-1 -0x1.5b403af9711e8p-3
0x1.e4f85f0e582c0p-1 0x1.4b57c729ba5e0p-3
0x1.9d6aa71f9cb18p-1 0x1.9fdf01e9551c8p-1
0x1.8p-1 0x1.8p-1
0x1.f41e5c48bb0b8p-2 -0x1.2df365dba47ecp-1
EOF
    ../compare.sh FULL 2 3 float x input.txt
}

test3() {
    new_env test3
    ## Denormal tests for range 2, precision 23
    cat >input.txt <<EOF
-0x1.eee201b1c85d4p-1 0x1.22034fafd4a10p-4
EOF
    ../compare.sh FULL 2 23 float x input.txt
}

test4() {
    new_env test4
    ## Subnormal halfway: -0.375 is exactly between -0.25 and -0.5, ties to even.
    cat >input.txt <<EOF
-0x1.a65cfc73c8a8ep-1 0x1.2e9e4e7fa94eap-1
EOF
    ../compare.sh FULL 2 2 double x input.txt
}

test5() {
    new_env test5
    ## Subnormal halfway: +0.1875 is exactly between +0.125 and +0.25, ties to even.
    cat >input.txt <<EOF
0x1.e6527affea0d8p-3 0x1.959edd6d29896p-1
EOF
    ../compare.sh FULL 2 3 double x input.txt
}

test6() {
    new_env test6
    ## Denormal tests for range 11, precision 2
    cat >input.txt <<EOF
-0x0.5c8e727e02168p-1022 -0x0.ffd401e0541ecp-1022
EOF
    ../compare.sh PB 11 2 double - input.txt
}

test7() {
    new_env test7
    ## Denormal tests for range 11, precision 2
    cat >input.txt <<EOF
-0x0.5c8e727e02168p-1022 -0x0.ffd401e0541ecp-1022
EOF
    ../compare.sh OB 11 2 double - input.txt
}

test8() {
    new_env test8
    ## Denormal tests for range 11, precision 2
    cat >input.txt <<EOF
-0x0.5c8e727e02168p-1022 -0x0.ffd401e0541ecp-1022
EOF
    ../compare.sh FULL 11 2 double - input.txt
}


export -f new_env
for i in {1..8}; do
    export -f test$i
done


parallel test{} ::: {1..8}
