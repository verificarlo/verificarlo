#!/bin/bash
# Compile tchebychev.c using Verificarlo
# Tchebychev polynomial becomes unstable around 1, when computed with
# single precision

set -e
export LC_ALL=C

# Check all arguments
if [ "$#" -ne 3 ]; then
  echo "usage: run.sh [EXPANDED | FACTORED | HORNER | COMPHORNER] [FLOAT | DOUBLE] vprecision"
  echo "       vprecision is Verificarlo Virtual Precision (an integer between 0 and 53)"
  exit 1
fi

METHOD=$1
REAL=$2
export VERIFICARLO_PRECISION=$3

# Check method
case "${METHOD}" in
    EXPANDED) ;;
    FACTORED) ;;
    HORNER) ;;
    COMPHORNER) ;;
    *)
	echo "Inexisting method "$1", choose between [EXPANDED | FACTORED | HORNER | COMPHORNER]"
	exit 1
esac

# Remove this block when you implement COMPHORNER
if  [[ "${METHOD}" == "COMPHORNER" ]];
then
  echo "COMPHORNER is not implemented"
  exit 1
fi

# Check real
case "${REAL}" in
    FLOAT) ;;
    DOUBLE) ;;
    *)
	echo "Inexisting precision "${REAL}", choose between [FLOAT | DOUBLE]"
	exit 1
esac

# Print options
echo "Verificarlo Precision = $VERIFICARLO_PRECISION, Real Type = $REAL, Method = $METHOD"

# Compile source code with verificarlo
verificarlo -D ${REAL} tchebychev.c -o tchebychev

# Run 20 iterations of tchebychev for all values x in [.5:1.0:.001]
# producing a .tab file with three columns:
#   - i: sample number
#   - x: input value
#   - T: polynomial evaluation on x, T(x)
echo "i x t" > ${METHOD}-${REAL}.tab
for x in $(seq 0.0 0.01 1.0); do
    for i in $(seq 1 20); do
      echo $i $(./tchebychev $x $METHOD) >> ${METHOD}-${REAL}.tab
    done
done

# Plot the samples
python plot.py ${METHOD}-${REAL}.tab $VERIFICARLO_PRECISION&
exit
