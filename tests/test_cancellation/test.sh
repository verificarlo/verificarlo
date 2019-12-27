verificarlo -D FLOAT test_canc_detect.c -o test_canc_detect -Wall -lm -O3 -ftree-vectorize
verificarlo -D FLOAT test_canc_exp.c -o test_canc_exp -Wall -lm -O3 -ftree-vectorize
rm -f OUTPUT/output_canc_detect_FLOAT
rm -f OUTPUT/output_canc_exp_FLOAT
for i in {0..22}; do
	export VFC_BACKENDS="libinterflop_cancellation.so --precision $i"
    ./test_canc_detect >> OUTPUT/output_canc_detect_FLOAT
    ./test_canc_exp >> OUTPUT/output_canc_exp_FLOAT
done

verificarlo -D DOUBLE test_canc_detect.c -o test_canc_detect -Wall -lm -O3 -ftree-vectorize
verificarlo -D DOUBLE test_canc_exp.c -o test_canc_exp -Wall -lm -O3 -ftree-vectorize
rm -f OUTPUT/output_canc_detect_DOUBLE
rm -f OUTPUT/output_canc_exp_DOUBLE
for i in {0..51}; do
	export VFC_BACKENDS="libinterflop_cancellation.so --precision $i"
    ./test_canc_detect >> OUTPUT/output_canc_detect_DOUBLE
    ./test_canc_exp >> OUTPUT/output_canc_exp_DOUBLE
done

./compare