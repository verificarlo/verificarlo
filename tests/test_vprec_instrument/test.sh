#!/bin/bash
#set -e

verificarlo-c test_mantissa.c -o test_mantissa --inst-func -lm

# mode none
rm -f output.txt

echo "--------------------------------------------------------------" >> output.txt
echo "							 Mantissa 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
printf "\n------------------- Instrumentation = None -------------------\n" >> output.txt

export VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=config.txt"
./test_mantissa 52 23 >> /dev/null

double_arr=(1 26 51)
float_arr=(1 11 22)

for i in 0 1 2; do
	./set_input_file config.txt ${double_arr[$i]} 11 ${float_arr[$i]} 8 Ffloat/powf main/Fdouble main/Ffloat Fdouble/pow
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	printf "\n	InBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	printf "\n	OutBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
done

# mode arguments

printf "\n---------------- Instrumentation = Arguments -----------------\n" >> output.txt

for i in 0 1 2; do
	./set_input_file config.txt ${double_arr[$i]} 11 ${float_arr[$i]} 8 Ffloat/powf main/Fdouble main/Ffloat Fdouble/pow
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	printf "\n	InBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	printf "\n	OutBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
done

# mode operations

printf "\n--------------- Instrumentation = Operations -----------------\n" >> output.txt

for i in 0 1 2; do
	./set_input_file config.txt ${double_arr[$i]} 11 ${float_arr[$i]} 8 Ffloat/powf main/Fdouble main/Ffloat Fdouble/pow
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	printf "\n	InBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	printf "\n	OutBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
done

# mode all

printf "\n------------------ Instrumentation = All --------------------\n" >> output.txt

for i in 0 1 2; do
	./set_input_file config.txt ${double_arr[$i]} 11 ${float_arr[$i]} 8 Ffloat/powf main/Fdouble main/Ffloat Fdouble/pow
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	printf "\n	InBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	printf "\n	OutBound Rounding at bit of the mantissa ${double_arr[$i]} in double precision and at bit ${float_arr[$i]} in single precision\n\n" >> output.txt
	./test_mantissa ${double_arr[$i]} ${float_arr[$i]} >> output.txt
done

verificarlo-c test_exponent.c -o test_exponent --inst-func -lm

echo "--------------------------------------------------------------" >> output.txt
echo "							 Exponent 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
echo "" >> output.txt

export VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=config.txt"
./test_exponent 52 23 >> /dev/null

double_arr=(11 10)
float_arr=(8 7)

# mode none

printf "\n------------------- Instrumentation = None -------------------\n" >> output.txt

for i in 0 1; do
	./set_input_file config.txt 52 ${double_arr[$i]} 23 ${float_arr[$i]} main/Fdouble main/Ffloat
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ib"
	printf "\n	InBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=none --mode=ob"
	printf "\n	OutBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
done

# mode arguments

printf "\n---------------- Instrumentation = Arguments -----------------\n" >> output.txt

for i in 0 1; do
	./set_input_file config.txt 52 ${double_arr[$i]} 23 ${float_arr[$i]} main/Fdouble main/Ffloat
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ib"
	printf "\n	InBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=arguments --mode=ob"
	printf "\n	OutBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
done

# mode operations

printf "\n--------------- Instrumentation = Operations -----------------\n" >> output.txt

for i in 0 1; do
	./set_input_file config.txt 52 ${double_arr[$i]} 23 ${float_arr[$i]} main/Fdouble main/Ffloat
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ib"
	printf "\n	InBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=operations --mode=ob"
	printf "\n	OutBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
done

# mode all

printf "\n------------------ Instrumentation = All --------------------\n" >> output.txt

for i in 0 1; do
	./set_input_file config.txt 52 ${double_arr[$i]} 23 ${float_arr[$i]} main/Fdouble main/Ffloat
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ib"
	printf "\n	InBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
	export VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=config.txt --instrument=all --mode=ob"
	printf "\n	OutBound Rounding of the exponent at bit ${double_arr[$i]} in double precision and ${float_arr[$i]} in simple precision\n\n" >> output.txt
	./test_exponent >> output.txt
done

echo "--------------------------------------------------------------" >> output.txt
echo "							 Range 							" >> output.txt
echo "--------------------------------------------------------------" >> output.txt
echo "" >> output.txt

verificarlo-c test_range.c -o test_range --inst-func -lm

export VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=config.txt"

printf "Functions Fdouble and Ffloat take real numbers from 1 to 102 and from -1 to -102 and subtract 10 \n\n" >> output.txt

./test_range
./print_range config.txt main/Ffloat main/Fdouble >> output.txt

if [ $(diff -U 0 result.txt output.txt | grep ^@ | wc -l) == 0 ]
then 
	exit 0
else 
	echo $(diff -U 0 result.txt output.txt | grep ^@ | wc -l)
	exit 1
fi

