flang -c vfc_probes_test.f90
flang vfc_probes_test.o libvfc_probes.so -o test_fortran

VFC_PROBES_OUTPUT="tmp.csv" ./test_fortran

if ls *.csv; then
    echo "CSV file found, SUCCESS"
    exit 0
else
    echo "CSV file not found, FAILURE"
    exit 1
fi
