//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#ifdef TIME
#define TEST N_TEST+1

#if defined(__GNUC__) && defined(__i386__)
	static inline uint64_t rdtsc()
	{
		uint64_t clk;
		__asm__ volatile ("rdtsc" : "=A" (clk));
		return clk;
	}
#elif defined(__GNUC__) && defined(__x86_64__)

	static inline uint64_t rdtsc()
	{
		uint32_t hi, lo;
		__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
		return lo | ((uint64_t)hi << 32);
	}
	
#else
#warning "No TSC on this platform"
static inline uint64_t rdtsc()
{
   return 0ULL;
}
#endif


#endif

#ifdef PAPI
  #ifndef TIME
	#define TEST N_TEST+1
  #endif
#else
  #ifndef TIME
    #define TEST 1
  #endif
#endif

#define Test_macro_WarmUp(func,n_,R_){\
		init(&(P[0]), n_);\
		memcpy(P[0],Pinit,(n_)*sizeof(double));\
		R_ = func(P[0], n_);\
		free(P[0]);\
}

#ifdef PAPI_FLOPS
#define Test_macro_PAPI_FLOPS(func,n_){\
	for(i=1;i<TEST;i++){\
			init(&(P[i]), n_);\
			memcpy(P[i],Pinit,(n_)*sizeof(double));}\
		for(i=1;i<TEST;i++){\
			flops=-1;\
			PAPI_flops(&real_time, &proc_time, &flops, &Mflops);\
			func(P[i], n_);\
			PAPI_flops(&real_time, &proc_time, &flops, &Mflops);\
			printf("time: %f Nfops: %lld Mflops: %f\n", proc_time, flops, Mflops);}\
		for(i=1;i<TEST;i++) free(P[i]);\
}	

#endif

#ifdef PAPI_TEST
#define Test_macro_PAPI_TEST(func,n_){\
	int Events[2] = { PAPI_TOT_CYC, PAPI_FP_INS };\
	long_long values[2];\
	for(i=1;i<TEST;i++){\
			init(&(P[i]), n_);\
			memcpy(P[i],Pinit,(n_)*sizeof(double));}\
	for(i=1;i<TEST;i++){\
		PAPI_start_counters(Events, 2);\
		func(P[i], n_);\
		PAPI_stop_counters(values, 2);\
		printf("Cycles: %lld, Nfops: %lld Mflops: %f\n",values[0], values[1], (1.*values[1])/(1.*values[0])*3e9);}\
	for(i=1;i<TEST;i++) free(P[i]);\
}
#endif

#ifdef PAPI_FLIPS
#define Test_macro_PAPI_FLIPS(func,n_){\
	for(i=1;i<TEST;i++){\
			init(&(P[i]), n_);\
			memcpy(P[i],Pinit,(n_)*sizeof(double));}\
		for(i=1;i<TEST;i++){\
			flops=-1;\
			PAPI_flips(&real_time, &proc_time, &flops, &Mflops);\
			func(P[i], n_);\
			PAPI_flips(&real_time, &proc_time, &flops, &Mflops);\
			printf("time: %f Nfips: %lld Mflips: %f\n", proc_time, flops, Mflops);}\
		for(i=1;i<TEST;i++) free(P[i]);\
}	

#endif

#ifdef PAPI_IPC
#define Test_macro_PAPI_IPC(func,n_){\
	for(i=1;i<TEST;i++){\
		init(&(P[i]), n_);\
		memcpy(P[i],Pinit,(n_)*sizeof(double));}\
	for(i=1;i<TEST;i++){\
		ins=-1;\
		PAPI_ipc(&real_time, &proc_time, &ins, &ipc);\
		func(P[i], n_);\
		PAPI_ipc(&real_time, &proc_time, &ins, &ipc);\
		printf("time: %f N_ins: %lld IPC: %f\n", proc_time, ins, ipc);}\
	for(i=1;i<TEST;i++) free(P[i]);\
}
#endif

#ifdef TIME_FILE_OUTPUT
#define TIME_FILE_OUTPUT_M fprintf(outfileN[i_n],"%7.7f ",t);\
		fprintf(outfileC[i_c],"%7.7f ",t);\
		fprintf(file3D,"%7.7f ",t);
#else
#define TIME_FILE_OUTPUT_M 
#endif

#ifdef TIME_STD_OUTPUT
	#define TIME_STD_OUTPUT_M printf("%7.7f Mcycles\n",t); 
#else
    #define TIME_STD_OUTPUT_M
#endif

#ifdef TIME
#define	Test_macro_TIME(func,n_){\
	for(i=1;i<TEST;i++){\
			init(&(P[i]), n_);\
			memcpy(P[i],Pinit,(n_)*sizeof(double));}\
	t=1e30;\
	double temp;\
	for(i=1;i<TEST;i++){\
			t1=rdtsc();\
			func(P[i], n_);\
			t2=rdtsc();\
			t2=t2-t1;\
			temp=((double)(t2))/1e6;\
			t=temp<t?temp:t;\
			TIME_STD_OUTPUT_M\
		}\
		TIME_FILE_OUTPUT_M\
		for(i=1;i<TEST;i++) free(P[i]);\
}		
#endif
