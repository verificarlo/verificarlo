set terminal postscript eps color

set title "Double and Accurate  Sum Timing for N=10e5"

set xlabel "conditionement"
set logscale y
set ylabel "time (ms)"

set output "full2D_N1000000_logTime.eps"
plot "../DATAout/outFile.N1000000.out.data" using 1:2 with lines title "AccSum", "../DATAout/outFile.N1000000.out.data" using 1:3 with lines title "FastAccSum" ,"../DATAout/outFile.N1000000.out.data" using 1:4 with lines  title "Sum2","../DATAout/outFile.N1000000.out.data" using 1:5 with lines title "DDSum

unset logscale y
set output "full2D_N1000000.eps"
plot "../DATAout/outFile.N1000000.out.data" using 1:2 with lines title "AccSum", "../DATAout/outFile.N1000000.out.data" using 1:3 with lines title "FastAccSum" ,"../DATAout/outFile.N1000000.out.data" using 1:4 with lines  title "Sum2","../DATAout/outFile.N1000000.out.data" using 1:5 with lines title "DDSum



set title "Double and Accurate  Sum Timing for C=10e96"

set logscale x
set xlabel "data size"
set logscale y
set ylabel "time (ms)"

set output "full2D_C96_logTime.eps"
plot "../DATAout/outFile.C96.out.data" using 1:2 with lines title "AccSum", "../DATAout/outFile.C96.out.data" using 1:3 with lines title "FastAccSum" ,"../DATAout/outFile.C96.out.data" using 1:4 with lines  title "Sum2","../DATAout/outFile.C96.out.data" using 1:5 with lines title "DDSum

unset logscale y
set output "full2D_C96.eps"
plot "../DATAout/outFile.C96.out.data" using 1:2 with lines title "AccSum", "../DATAout/outFile.C96.out.data" using 1:3 with lines title "FastAccSum" ,"../DATAout/outFile.C96.out.data" using 1:4 with lines  title "Sum2","../DATAout/outFile.C96.out.data" using 1:5 with lines title "DDSum


set title "Double and Accurate Sum Timing"

set xlabel "data size"
set ylabel "conditionement"
set zlabel "time (ms)"
set logscale x


set output "full3D.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:3 with lines title "AccSum",\
      	        "../DATAout/outFile.3D.out.data"  using 1:2:4 with lines title "FastAccSum",\
 		       "../DATAout/outFile.3D.out.data"  using 1:2:5 with lines title "Sum2", \
		              "../DATAout/outFile.3D.out.data"  using 1:2:6 with lines title "DDSum"

set title "Accurate Sum Timing"

set logscale z
set output "acc3DfullRange_logTime.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:3 with lines title "AccSum",\
      	        "../DATAout/outFile.3D.out.data"  using 1:2:4 with lines title "FastAccSum"

set title "Accurate Sum Timing"
set output "acc3D_Nmin100000_logTime.eps"
splot [100000:] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:3 with lines title "AccSum",\
      		       "../DATAout/outFile.3D.out.data"  using 1:2:4 with lines title "FastAccSum"


set title "Accurate Sum Speed-Up"

unset logscale z
zval=1
set clabel 'SU=%g'
set contour #surface
set cntrparam levels discrete zval

set view 110, 30, 1, 1
set pm3d implicit at sb
set output "SUacc3D_Nmin100000.eps"
splot [100000:] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$4) with lines title "AccSum/FastAccSum"
set output "SUacc3D.eps"
splot [] [] [] "../DATAout/outFile.3D.out.data"  using 1:2:($3/$4) with lines title "AccSum/FastAccSum"


