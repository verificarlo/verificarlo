set terminal postscript eps color
set lmargin 2
set xlabel "data size"
set ylabel "cond. (10E )"
set zlabel "SpeedUp"
set logscale x

zval=1.0
set clabel 'SU=%g'
set contour #surface
set cntrparam levels discrete zval

set view 110, 30, 1, 1
set pm3d implicit at sb 

set output "SUacc3D_ACCvsFASTACC_icc.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$4) with lines title "t(AccSum)/t(FastAccSum)"


