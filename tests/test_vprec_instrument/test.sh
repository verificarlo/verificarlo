#!/bin/bash
#set -e

verificarlo-c test_mantissa.c -o test_mantissa --inst-func -lm

# mode none
rm -f output.txt

echo "--------------------------------------------------------------" >> output.txt
echo "							 Mantissa 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
echo "" >> output.txt
echo "--------------------------- None -----------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	$i	11	23	$i	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	1	1
output:	return_value	0	23	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	0	0
output:	return_value	1	$i	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	0	0
output:	return_value	0	23	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	1	1
output:	return_value	1	$i	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test_mantissa $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test_mantissa $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	1	1
output:	return_value	0	$i	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	0	0
output:	return_value	1	52	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	0	0
output:	return_value	0	$i	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	1	1
output:	return_value	1	52	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test_mantissa 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test_mantissa 52 $i >> output.txt
done

# mode arguments

echo "----------------------------- Arguments ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	1	1
output:	return_value	0	23	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	0	0
output:	return_value	1	$i	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	0	0
output:	return_value	0	23	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	1	1
output:	return_value	1	$i	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test_mantissa $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test_mantissa $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	1	1
output:	return_value	0	$i	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	0	0
output:	return_value	1	52	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	0	0
output:	return_value	0	$i	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	1	1
output:	return_value	1	52	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test_mantissa 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test_mantissa 52 $i >> output.txt
done

# mode operations

echo "----------------------------- Operations ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	1	1
output:	return_value	0	23	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	0	0
output:	return_value	1	$i	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	0	0
output:	return_value	0	23	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	1	1
output:	return_value	1	$i	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test_mantissa $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test_mantissa $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	1	1
output:	return_value	0	$i	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	0	0
output:	return_value	1	52	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	0	0
output:	return_value	0	$i	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	1	1
output:	return_value	1	52	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test_mantissa 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test_mantissa 52 $i >> output.txt
done

# mode all

echo "----------------------------- All ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 1 26 52; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	1	1
output:	return_value	0	23	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	0	0
output:	return_value	1	$i	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	$i	11	23	8	2	1	1
input:	parameter_1	0	23	8	0	0
input:	parameter_2	0	23	8	0	0
output:	return_value	0	23	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	$i	11	23	8	2	1	1
input:	parameter_1	1	$i	11	0	0
input:	parameter_2	1	$i	11	1	1
output:	return_value	1	$i	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test_mantissa $i 23 >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test_mantissa $i 23 >> output.txt
done

echo "Float: " >> output.txt

for i in 1 11 23; do
	echo \
"test_mantissa.c/Ffloat/powf/64/27	1	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	1	1
output:	return_value	0	$i	8	0	0
test_mantissa.c/main/Fdouble/84/47	0	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	0	0
output:	return_value	1	52	11	0	0
test_mantissa.c/main/Ffloat/85/49	0	0	1	0	52	11	$i	8	2	1	1
input:	parameter_1	0	$i	8	0	0
input:	parameter_2	0	$i	8	0	0
output:	return_value	0	$i	8	0	0
test_mantissa.c/Fdouble/pow/47/10	1	0	0	1	52	11	$i	8	2	1	1
input:	parameter_1	1	52	11	0	0
input:	parameter_2	1	52	11	1	1
output:	return_value	1	52	11	0	0" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test_mantissa 52 $i >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test_mantissa 52 $i >> output.txt
done

echo "--------------------------------------------------------------" >> output.txt
echo "							 Exponent 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
echo "" >> output.txt

verificarlo-c test_exponent.c -o test_exponent --inst-func -lm

# mode none

echo "--------------------------- None -----------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 11 10; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	0	23	8	1	1
output:	return_value	0	23	8	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	$i	23	8	1	1	
input:	parameter_1	52	$i	1	1
output:	return_value	1	52	$i	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test_exponent >> output.txt
done

echo "Float: " >> output.txt

for i in 8 7; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	0	23	$i	1	1
output:	return_value	0	23	$i	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	1	52	11	1	1
output:	return_value	1	52	11	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	./test_exponent >> output.txt
done

# mode operations

echo "----------------------------- Operations ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 11 10; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	0	23	8	1	1
output:	return_value	0	23	8	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	1	52	$i	1	1
output:	return_value	1	52	$i	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test_exponent >> output.txt
done

echo "Float: " >> output.txt

for i in 8 7; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	0	23	$i	1	1
output:	return_value	0	23	$i	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	1	52	11	1	1
output:	return_value	1	52	11	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	./test_exponent >> output.txt
done

# mode arguments

echo "----------------------------- Arguments ---------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 11 10; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	0	23	8	1	1
output:	return_value	0	23	8	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	1	52	$i	1	1
output:	return_value	1	52	$i	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test_exponent >> output.txt
done

echo "Float: " >> output.txt

for i in 8 7; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	0	23	$i	1	1
output:	return_value	0	23	$i	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	1	52	11	1	1
output:	return_value	1	52	11	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	./test_exponent >> output.txt
done

# mode all

echo "---------------------------- All -----------------------------" >> output.txt

echo "Double: " >> output.txt

for i in 11 10; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	0	23	8	1	1
output:	return_value	0	23	8	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	$i	23	8	1	1	1
input:	parameter_1	1	52	$i	1	1
output:	return_value	1	52	$i	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test_exponent >> output.txt
done

echo "Float: " >> output.txt

for i in 8 7; do
	echo \
"test_exponent.c/main/Ffloat/67/28	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	0	23	$i	1	1
output:	return_value	0	23	$i	1	1
test_exponent.c/main/Fdouble/71/31	0	0	1	1	52	11	23	$i	1	1	1
input:	parameter_1	1	52	11	1	1
output:	return_value	1	52	11	1	1" > config.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	./test_exponent >> output.txt
done

echo "--------------------------------------------------------------" >> output.txt
echo "							 Range 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
echo "" >> output.txt

verificarlo-c test_range.c -o test_range --inst-func -lm

export VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=tmp_config.txt"

./test_range

cat tmp_config.txt >> output.txt


if [ $(diff -U 0 result.txt output.txt | grep ^@ | wc -l) == 0 ]
then 
	exit 0
else 
	echo $(diff -U 0 result.txt output.txt | grep ^@ | wc -l)
	exit 1
fi

