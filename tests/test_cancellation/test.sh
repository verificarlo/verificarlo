verificarlo -D FLOAT test_canc_detect.c -o test_canc_detect -Wall 
rm -f OUTPUT/output_canc_detect_FLOAT
rm -f OUTPUT/output_canc_exp_FLOAT
for i in `seq 0 22`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i"
    ./test_canc_detect >> OUTPUT/output_canc_detect_FLOAT
done

verificarlo -D DOUBLE test_canc_detect.c -o test_canc_detect -Wall 
rm -f OUTPUT/output_canc_detect_DOUBLE
rm -f OUTPUT/output_canc_exp_DOUBLE
for i in `seq 0 51`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i"
    ./test_canc_detect >> OUTPUT/output_canc_detect_DOUBLE
done

make compare
./compare
