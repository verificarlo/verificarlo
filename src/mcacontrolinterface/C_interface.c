#include <mcalib.h>

void set_mca_precision(int * new_precision){
	MCALIB_T = *new_precision
}

int get_mca_precision(){
	return MCALIB_T;
}


