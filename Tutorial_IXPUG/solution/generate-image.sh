mkdir -p ../figs/
export VERIFICARLO_BACKEND=QUAD
export VERIFICARLO_MCAMODE=MCA
for METHOD in EXPANDED FACTORED HORNER COMPHORNER; do
  REAL="DOUBLE"
  for PREC in 24 53; do
    make clean
    ./run-images.sh $METHOD $REAL $PREC
    cp $METHOD-$REAL-$PREC.pdf ../figs/
  done
  REAL="FLOAT"
  PREC=24
  make clean
  ./run-images.sh $METHOD $REAL $PREC
  cp $METHOD-$REAL-$PREC.pdf ../figs/
done

for METHOD in EXPANDED FACTORED HORNER; do
  REAL="DOUBLE"
  for PREC in 24 53; do
    make clean
    ./run-zoomed-images.sh $METHOD $REAL $PREC
    cp $METHOD-$REAL-$PREC.pdf ../figs/$METHOD-$REAL-$PREC-zoom.pdf
  done
done

export VERIFICARLO_MCAMODE=RR
make clean
./run-images.sh COMPHORNER DOUBLE 53
cp COMPHORNER-DOUBLE-53.pdf ../figs/

make clean
./run-images.sh COMPHORNER FLOAT 24
cp COMPHORNER-FLOAT-24.pdf ../figs/


