set terminal postscript eps color
set lmargin 2
set xlabel "data size"
set ylabel "conditionement (10e...)"
set zlabel "SpeedUp"
set logscale x

zval=1.0
set clabel 'SU=%g'
set contour #surface
set cntrparam levels discrete zval

set view 110, 30, 1, 1
set pm3d implicit at sb 

set output "SUacc3D_ACCvsACCVECT.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$4) with lines title "AccSum/AccSumVect"
set output "SUacc3D_ACCvsFACC.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$5) with lines title "AccSum/FastAccSum"
set output "SUacc3D_ACCvsFACCU.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$6) with lines title "AccSum/FastAccSumUnrolled"
set output "SUacc3D_ACCVECTvsFACCU.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($4/$6) with lines title "AccSumVect/FastAccSumUnrolled"


