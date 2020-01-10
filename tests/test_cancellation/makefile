all : clean test_canc_detect compare
	./compare
	#sudo ./test.sh
	#cat ./test.log
test_canc_detect: test_canc_detect.c
	gcc test_canc_detect.c -o test_canc_detect -Wall -O3 -ftree-vectorize

compare: compare.c
	gcc compare.c -o compare -Wall -O3 -ftree-vectorize

clean:
	rm -f compare test_canc_detect
