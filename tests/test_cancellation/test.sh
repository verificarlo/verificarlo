SEED=5929

export VFC_BACKENDS_SILENT_LOAD="True"

verificarlo-c test_float.c -o test_float

rm -f output.txt

for i in `seq 0 23`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i --seed=$SEED --warning=WARNING"
  ./test_float $i >> output.txt 2>&1 
done

verificarlo-c test_double.c -o test_double 

for i in `seq 0 52`; do
	export VFC_BACKENDS="libinterflop_cancellation.so --tolerance $i --seed=$SEED --warning=WARNING"
  ./test_double $i >> output.txt 2>&1 
done

echo $(diff -U 0 result.txt output.txt | grep ^@ | wc -l)

if [ $(diff -U 0 result.txt output.txt | grep ^@ | wc -l) == 0 ]
then 
	exit 0
else 
	exit 1
fi
