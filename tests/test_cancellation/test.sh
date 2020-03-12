SEED=5929

verificarlo -D FLOAT test_canc_detect.c -o test_canc_detect -Wall
rm -f OUTPUT/output_canc_detect_FLOAT
for i in `seq 0 22`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i --seed=$SEED"
  ./test_canc_detect >> OUTPUT/output_canc_detect_FLOAT
done

verificarlo -D DOUBLE test_canc_detect.c -o test_canc_detect -Wall
rm -f OUTPUT/output_canc_detect_DOUBLE
for i in `seq 0 51`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i --seed=$SEED"
  ./test_canc_detect >> OUTPUT/output_canc_detect_DOUBLE
done

rm -f test.log
diff  OUTPUT/output_canc_detect_DOUBLE RES/res_canc_detect_DOUBLE >> test.log
diff  OUTPUT/output_canc_detect_FLOAT RES/res_canc_detect_FLOAT >> test.log

res=`wc -l < test.log`

if [ $res != 0 ] ; then
echo "error"
exit 1
else
echo "ok"
fi
