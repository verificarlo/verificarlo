#include<stdio.h>
#include <mcalib.h>
__attribute__((constructor)) void begin (void)
{
    _mca_seed();
    MCALIB_T = 24;
    MCALIB_OP_TYPE = MCALIB_MCA;
}

typedef double double2 __attribute__((ext_vector_type(2)));

double2 _2xdoubleadd(double2 a, double2 b) {
    double2 c;

    c[0] = _doubleadd(a[0],b[0]);
    c[1] = _doubleadd(a[1],b[1]);
    return c;
}

double2 _2xdoublemul(double2 a, double2 b) {
    double2 c;

    c[0] = _doublemul(a[0],b[0]);
    c[1] = _doublemul(a[1],b[1]);
    return c;
}
