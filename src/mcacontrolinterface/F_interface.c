#include <mcalib.h>

#ifdef __cplusplus
	//remove name mangling	
	extern"C" {
#endif


void _F_CHANGE_MCA_PRECISION(int * new_precision){
	MCALIB_T = *new_precision
}

void _F_GET_MCA_PRECISION(int * precision){
	precision=MCALIB_T
}

#ifdef __cplusplus
	//end remove name mangling
	}
#endif

