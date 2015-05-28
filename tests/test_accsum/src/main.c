//######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#####################################

//--TODO -- allocation of variables for results checking should be automatize threw a linked list
	//each bench add a node with its name, and its result
	//the final output dump out this list

//--TODO-- meta-macro for double preprocessing	
	//each bench and option macro should be generated from bench.conf file in a first preprocessor phase
	//the second phase genrates the final main.c code
	
#include "bench.conf"
#include "all_header.h"
#include "test_macro.h"

int main(int argc, char *argv[])
{
	double c;
	//double X=1.1;
	double *Pinit,*P[TEST];
	int i,i_c,i_n;
	int SIZE=0,SIZEb=0;
	double C[NB_MAX_C];
	int C_EXP[NB_MAX_C];
	unsigned long int N[NB_MAX_N];
	

#ifdef PAPI_FLOPS
	float real_time, proc_time, Mflops;
	long long int flops;
#endif

#ifdef PAPI_FLIPS
	float real_time, proc_time, Mflops;
	long long int flops;
#endif	

#ifdef PAPI_TIME
	float real_time, proc_time, ipc;
	long long int ins;
#endif	
	
#ifdef TIME	
	uint64_t t1,t2;
	double t;
#endif

	initTabC();
	initTabN();	
	
#if defined(TIME) && defined(TIME_FILE_OUTPUT)
	FILE * outfileC[NB_C];
	FILE * outfileN[NB_N];
	FILE * file3D;
	//older file cleaning???
	//use w+, file are created if doesn't exist
	char* filename=malloc(65*sizeof(char));
	for(i_n=0;i_n<NB_N;i_n++)
	{	
		sprintf(filename,"./DATAout/outFile.N%lu.out.data",N[i_n]);
		outfileN[i_n]=fopen(filename,"w+");
	}
	
	for(i_c=0;i_c<NB_C;i_c++)
	{
		sprintf(filename,"./DATAout/outFile.C%d.out.data",C_EXP[i_c]);
		outfileC[i_c]=fopen(filename,"w+");
	}	
	sprintf(filename,"./DATAout/outFile.3D.out.data");
	file3D=fopen(filename,"w+");
	
#endif	
assert(NB_N*NB_N_STRIDE+NB_N_MIN<=NB_MAX_N);	
for(i_n=NB_N_MIN;i_n<NB_N*NB_N_STRIDE+NB_N_MIN;i_n+=NB_N_STRIDE)
	{	
	SIZE=N[i_n];
	
	
	
	for(i_c=0;i_c<NB_C;i_c++)
	{
		if (VERBOSE_TEST>0) printf("------------------------------------------------\n");
		
		//init P as array of 0
		init(&Pinit, SIZE);
		
		//init rand from libc needed by gensum
		if (VERBOSE_TEST>0) printf("---- Generating %lu data with C==%.4e \n",N[i_n],C[i_c]);
		
		if (SIZE<100)
		{
			srand(3);
			GenSum(Pinit, &c, SIZE, C[i_c]);
		}
		else	
			genSum_fromFile(Pinit, &c, SIZE, C_EXP[i_c]);
		
		if (VERBOSE_TEST>0) printf("---- Test done with C=%.4e \n",c);
		if (VERBOSE_TEST>0) printf("------------------------------------------------\n\n");
		
		#if defined(TIME) && defined(TIME_FILE_OUTPUT)
		fprintf(outfileN[i_n],"%d ",C_EXP[i_c]);
		fprintf(outfileC[i_c],"%lu ",N[i_n]);
		fprintf(file3D,"%lu ",N[i_n]);
		fprintf(file3D,"%d ",C_EXP[i_c]);
		#endif
		
		//------------------------------
		//----------AccSum test --------
		//------------------------------
#ifdef ACCSUM
		double R2;
		Test_macro_WarmUp(AccSumIn,SIZE,R2);
				
#ifdef TIME
		if (VERBOSE_TEST>0) printf("AccSum test tsc:\n");
		Test_macro_TIME(AccSumIn,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("AccSum test PAPI-C:\n");

#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(AccSumIn,SIZE);		
#endif		

#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(AccSumIn,SIZE);	
#endif		
		

#ifdef PAPI_TEST		
		Test_macro_PAPI_TEST(AccSumIn,SIZE);		
#endif
		
#endif
#endif		

		//------------------------------
		//----------AccSumVect test --------
		//------------------------------
#ifdef ACCSUMVECT
		double R12;
		Test_macro_WarmUp(AccSumVectIn,SIZE,R12);
				
#ifdef TIME
		if (VERBOSE_TEST>0) printf("AccSumVect test tsc:\n");
		Test_macro_TIME(AccSumVectIn,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("AccSumVect test PAPI-C:\n");

#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(AccSumVectIn,SIZE);		
#endif		


#ifdef PAPI_FLIPS		
		Test_macro_PAPI_FLIPS(AccSumVectIn,SIZE);		
#endif	


#ifdef PAPI_TEST		
		Test_macro_PAPI_TEST(AccSumVectIn,SIZE);		
#endif	

#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(AccSumVectIn,SIZE);	
#endif		
		
		
#endif
#endif		

		//------------------------------
		//----------AccSumPar test --------
		//------------------------------
#ifdef ACCSUMPAR
		double R13;
		Test_macro_WarmUp(AccSumParIn,SIZE,R13);
				
#ifdef TIME
		if (VERBOSE_TEST>0) printf("AccSumPar test tsc:\n");
		Test_macro_TIME(AccSumParIn,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("AccSumPar test PAPI-C:\n");

#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(AccSumParIn,SIZE);		
#endif		


#ifdef PAPI_FLIPS		
		Test_macro_PAPI_FLIPS(AccSumParIn,SIZE);		
#endif	


#ifdef PAPI_TEST		
		Test_macro_PAPI_TEST(AccSumParIn,SIZE);		
#endif	

#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(AccSumParIn,SIZE);	
#endif		
		
		
#endif
#endif	

		//---------------------------------------
		//----------FastAcc original test--------
		//---------------------------------------
#ifdef FASTACCSUM	
		double R1;
		Test_macro_WarmUp(FastAccSumIn,SIZE,R1);		
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSum test tsc:\n");
		Test_macro_TIME(FastAccSumIn,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSum test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumIn,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumIn,SIZE);	
	#endif	
	
#ifdef PAPI_TEST		
		Test_macro_PAPI_TEST(FastAccSumIn,SIZE);		
#endif
					
#endif

#endif
		
		//---------------------------------------
		//----------FastAcc_outlined_loops_ test--------
		//---------------------------------------
#ifdef FASTACCSUMoutlined	
		double R10;
		Test_macro_WarmUp(FastAccSum_outlined_loops_In,SIZE,R10);		
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSum outlined test tsc:\n");
		Test_macro_TIME(FastAccSum_outlined_loops_In,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSum outlined test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSum_outlined_loops_In,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSum_outlined_loops_In,SIZE);	
	#endif				
#endif

#endif

		//---------------------------------------
		//----------FastAccOpt test unroll=2--------
		//---------------------------------------
#ifdef FASTACCSUMopt2	
		double R7;
		//opt works only with odd entry size, so add a 0 at the end if needed
		SIZEb=SIZE+SIZE%2;
		Test_macro_WarmUp(FastAccSumOpt_unroll2_In,SIZEb,R7);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSumOptU2 test tsc:\n");
		Test_macro_TIME(FastAccSumOpt_unroll2_In,SIZEb);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSumOpt_U2 test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumOpt_unroll2_In,SIZEb);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumOpt_unroll2_In,SIZEb);	
	#endif	
	
#ifdef PAPI_TEST		
		Test_macro_PAPI_TEST(FastAccSumOpt_unroll2_In,SIZE);		
#endif
					
#endif

#endif

	//---------------------------------------
		//----------FastAccOpt outlined test unroll=2--------
		//---------------------------------------
#ifdef FASTACCSUMopt2_outlined	
		double R11;
		//opt works only with odd entry size, so add a 0 at the end if needed
		SIZEb=SIZE+SIZE%2;
		Test_macro_WarmUp(FastAccSumOpt_unroll2_outlined_loops_In,SIZEb,R11);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSumOptU2 outlined loops test tsc:\n");
		Test_macro_TIME(FastAccSumOpt_unroll2_outlined_loops_In,SIZEb);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSumOpt_U2 outlined loops test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumOpt_unroll2_outlined_loops_In,SIZEb);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumOpt_unroll2_outlined_loops_In,SIZEb);	
	#endif				
#endif

#endif


		//---------------------------------------
		//----------FastAccOpt test unroll=3--------
		//---------------------------------------
#ifdef FASTACCSUMopt3	
		double R9;
		//opt works only with multiple of 3 entry size, so add a 0 at the end if needed
		SIZEb=SIZE+SIZE%3;
		Test_macro_WarmUp(FastAccSumOpt_unroll3_In,SIZEb,R9);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSumOptU3 test tsc:\n");
		Test_macro_TIME(FastAccSumOpt_unroll3_In,SIZEb);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSumOpt_U3 test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumOpt_unroll3_In,SIZEb);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumOpt_unroll3_In,SIZEb);	
	#endif				
#endif

#endif


		//---------------------------------------
		//----------FastAccOpt test unroll=4--------
		//---------------------------------------
#ifdef FASTACCSUMopt4	
		double R8;
		//opt works only with multiple of 4 entry size, so add a 0 at the end if needed
		SIZEb=SIZE+SIZE%4;
		Test_macro_WarmUp(FastAccSumOpt_unroll2_In,SIZEb,R8);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSumOptU4 test tsc:\n");
		Test_macro_TIME(FastAccSumOpt_unroll4_In,SIZEb);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSumOpt_U4 test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumOpt_unroll4_In,SIZEb);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumOpt_unroll4_In,SIZEb);	
	#endif				
#endif

#endif


		//---------------------------------------
		//----------FastAcc b test--------
		//---------------------------------------
#ifdef FASTACCSUMb	
		double R6;	
		Test_macro_WarmUp(FastAccSumbIn,SIZE,R6);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("FastAccSumb test tsc:\n");
		Test_macro_TIME(FastAccSumbIn,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("FastAccSumb test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(FastAccSumbIn,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(FastAccSumbIn,SIZE);	
	#endif				
#endif

#endif


		//------------------------------
		//----------Sum2 test --------
		//------------------------------
#ifdef SUM2		
		double R3;
		Test_macro_WarmUp(Sum2,SIZE,R3);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("Sum2 test tsc:\n");
		Test_macro_TIME(Sum2,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("Sum2 test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(Sum2,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(Sum2,SIZE);	
	#endif				
#endif

#endif		
		
		//------------------------------
		//----------XBLAS DDsum test --------
		//------------------------------
#ifdef DDSUM
		double R4;
		Test_macro_WarmUp(DDSum,SIZE,R4);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("DDSum test tsc:\n");
		Test_macro_TIME(DDSum,SIZE);
#endif

#ifdef PAPI
		if (VERBOSE_TEST>0) printf("DDSum test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(DDSum,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(DDSum,SIZE);	
	#endif				
#endif

#endif		
		//------------------------------
		//----------opt XBLAS DDsum test --------
		//------------------------------
#ifdef DDSUMbis
		double R5;	
		Test_macro_WarmUp(DDSumBis,SIZE,R5);
		
#ifdef TIME
		if (VERBOSE_TEST>0) printf("DDSumBis test tsc:\n");
		Test_macro_TIME(DDSumBis,SIZE);
#endif


#ifdef PAPI
		if (VERBOSE_TEST>0) printf("DDSumBis test PAPI-C:\n");	
	#ifdef PAPI_FLOPS		
		Test_macro_PAPI_FLOPS(DDSumBis,SIZE);		
	#endif		
	#ifdef PAPI_IPC		
		Test_macro_PAPI_IPC(DDSumBis,SIZE);	
	#endif				
#endif

#endif


//------------------------------
//---------next line in outFiles--------
//------------------------------

#if defined(TIME) && defined(TIME_FILE_OUTPUT)
			fprintf(outfileN[i_n],"\n");
			fprintf(outfileC[i_c],"\n");
			fprintf(file3D,"\n");
#endif

//------------------------------
//---------Results check--------
//------------------------------

#ifdef RESULT_CHECK		
		
		if (VERBOSE_TEST>0) printf("\n");
		if (VERBOSE_TEST>0) printf("-------------------------------\n");
		if (VERBOSE_TEST>0) printf("--------  results check -------\n");
		if (VERBOSE_TEST>0) printf("-------------------------------\n");
		if (VERBOSE_TEST>0) printf("\n");
	#ifdef FASTACCSUM	
		if (VERBOSE_TEST>0) printf("   fastaccsum=%.17f\n",R1);
	#endif	
	#ifdef FASTACCSUMoutlined	
		if (VERBOSE_TEST>0) printf("   fastaccsumoutlined=%.17f\n",R10);
	#endif	
	#ifdef FASTACCSUMb	
		if (VERBOSE_TEST>0) printf("  fastaccsumb=%.17f\n",R6);
	#endif	
	#ifdef FASTACCSUMopt2	
		if (VERBOSE_TEST>0) printf("fastaccsum_U2=%.17f\n",R7);
	#endif
	#ifdef FASTACCSUMopt2_outlined	
		if (VERBOSE_TEST>0) printf("fastaccsum_U2_outlined=%.17f\n",R11);
	#endif
	#ifdef FASTACCSUMopt3	
		if (VERBOSE_TEST>0) printf("fastaccsum_U3=%.17f\n",R9);
	#endif
	#ifdef FASTACCSUMopt4	
		if (VERBOSE_TEST>0) printf("fastaccsum_U4=%.17f\n",R8);
	#endif	
	#ifdef ACCSUM	
		if (VERBOSE_TEST>0) printf("       accsum=%.17f\n",R2);
	#endif	
	#ifdef ACCSUMVECT	
		if (VERBOSE_TEST>0) printf("   accsumVect=%.17f\n",R12);
	#endif	
	#ifdef SUM2	
		if (VERBOSE_TEST>0) printf("         sum2=%.17f\n",R3);
	#endif	
	#ifdef DDSUM	
		if (VERBOSE_TEST>0) printf("       DDsum=%.17f\n",R4);
	#endif
	#ifdef DDSUMbis	
		if (VERBOSE_TEST>0) printf("  DDsumBis=%.17f\n",R5);
	#endif
	#ifdef ACCSUMPAR	
		if (VERBOSE_TEST>0) printf("   accsumPar=%.17f\n",R13);
	#endif	
		

#endif
		if (VERBOSE_TEST>0) printf("\n\n");

		free(Pinit);
		

		}//end cond loop
	
	
	#if defined(TIME) && defined(TIME_FILE_OUTPUT)
	fprintf(file3D,"\n");
	#endif	
	}//end N loop	
	
	
#if defined(TIME) && defined(TIME_FILE_OUTPUT)
	for(i_n=0;i_n<NB_N;i_n++)
	{	
		fclose(outfileN[i_n]);
	}
	
	for(i_c=0;i_c<NB_C;i_c++)
	{
		fclose(outfileC[i_c]);
	}
#endif
		
return 0;
}



