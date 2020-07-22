#!/bin/bash
#set -e

verificarlo-c test.c -o test --inst-func -lm

# mode none
rm -f output.txt

echo "--------------------------- None -----------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test.c/powf_64_26	1	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8
test.c/pow_47_9	1	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Fdouble_84_46	0	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Ffloat_85_48	0	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test.c/powf_64_26	1	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8
test.c/pow_47_9	1	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Fdouble_84_46	0	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Ffloat_85_48	0	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test 52 $i >> output.txt
done

# mode arguments

echo "----------------------------- Arguments ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test.c/powf_64_26	1	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8
test.c/pow_47_9	1	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Fdouble_84_46	0	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Ffloat_85_48	0	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test.c/powf_64_26	1	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8
test.c/pow_47_9	1	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Fdouble_84_46	0	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Ffloat_85_48	0	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test 52 $i >> output.txt
done

# mode operations

echo "----------------------------- Operations ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test.c/powf_64_26	1	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8
test.c/pow_47_9	1	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Fdouble_84_46	0	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Ffloat_85_48	0	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test.c/powf_64_26	1	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8
test.c/pow_47_9	1	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Fdouble_84_46	0	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Ffloat_85_48	0	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test 52 $i >> output.txt
done

# mode all

echo "----------------------------- All ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test.c/powf_64_26	1	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8
test.c/pow_47_9	1	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Fdouble_84_46	0	0	0	1	$i	11	23	8	2	1	1
input:	1	$i	11
input:	1	$i	11
output:	1	$i	11
test.c/Ffloat_85_48	0	0	1	0	$i	11	23	8	2	1	1
input:	0	23	8
input:	0	23	8
output:	0	23	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test.c/powf_64_26	1	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8
test.c/pow_47_9	1	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Fdouble_84_46	0	0	0	1	52	11	$i	8	2	1	1
input:	1	52	11
input:	1	52	11
output:	1	52	11
test.c/Ffloat_85_48	0	0	1	0	52	11	$i	8	2	1	1
input:	0	$i	8
input:	0	$i	8
output:	0	$i	8" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test 52 $i >> output.txt
done

if [ $(diff -U 0 result.txt output.txt | grep ^@ | wc -l) == 0 ]
then 
	exit 0
else 
	echo $(diff -U 0 result.txt output.txt | grep ^@ | wc -l)
	exit 1
fi