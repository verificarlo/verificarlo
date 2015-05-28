//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>



#include <dp_tools.h>

#include <assert.h>

#ifdef PAPI
//fix papi compilation feature with c99
typedef char * caddr_t;
#include "papiStdEventDefs.h"
#include "papi.h"
#endif

void init(double **, int );
double GenSum(double *, double *, unsigned int , double );
double genSum_fromFile(double *, double *, unsigned int , int);
double AccSumIn(double *, unsigned int );
double AccSumVectIn(double *, unsigned int );
double AccSumParIn(double *, unsigned int );
double GAccSumIn(double *, unsigned int );
double AccSum(double *, unsigned int );
double GAccSum(double *, unsigned int );


#ifdef FASTACCSUM
double FastAccSumIn(double *, unsigned int );
#endif
#ifdef FASTACCSUMoutlined
double FastAccSum_outlined_loops_In(double *, unsigned int );
#endif 
#ifdef FASTACCSUMb
double FastAccSumbIn(double *, unsigned int );
#endif
#ifdef FASTACCSUMopt2
double  FastAccSumOpt_unroll2_In(double *, unsigned int );
#endif
#ifdef FASTACCSUMopt2_outlined
double  FastAccSumOpt_unroll2_outlined_loops_In(double *, unsigned int );
#endif
#ifdef FASTACCSUMopt4
double  FastAccSumOpt_unroll4_In(double *, unsigned int );
#endif
#ifdef FASTACCSUMopt3
double  FastAccSumOpt_unroll3_In(double *, unsigned int );
#endif
#ifdef SUM2
double Sum2(double *, unsigned int ) ;
#endif
#ifdef DDSUM
double DDSum(double *, unsigned int );
#endif
#ifdef DDSUMbis
double DDSumBis(double *, unsigned int );
#endif


