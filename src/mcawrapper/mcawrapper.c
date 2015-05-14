#include<stdio.h>
#include <mcalib.h>
__attribute__((constructor)) void begin (void)
{
    _mca_seed();
    MCALIB_T = 40;
    MCALIB_OP_TYPE = MCALIB_MCA;
}

typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));

double2 _2xdoubleadd(double2 a, double2 b) {
    double2 c;

    c[0] = _doubleadd(a[0],b[0]);
    c[1] = _doubleadd(a[1],b[1]);
    return c;
}

double2 _2xdoublesub(double2 a, double2 b) {
    double2 c;

    c[0] = _doublesub(a[0],b[0]);
    c[1] = _doublesub(a[1],b[1]);
    return c;
}

double2 _2xdoublemul(double2 a, double2 b) {
    double2 c;

    c[0] = _doublemul(a[0],b[0]);
    c[1] = _doublemul(a[1],b[1]);
    return c;
}

double2 _2xdoublediv(double2 a, double2 b) {
    double2 c;

    c[0] = _doublediv(a[0],b[0]);
    c[1] = _doublediv(a[1],b[1]);
    return c;
}

double4 _4xdoubleadd(double4 a, double4 b) {
    double4 c;

    c[0] = _doubleadd(a[0],b[0]);
    c[1] = _doubleadd(a[1],b[1]);
    c[2] = _doubleadd(a[2],b[2]);
    c[3] = _doubleadd(a[3],b[3]);
    return c;
}

double4 _4xdoublesub(double4 a, double4 b) {
    double4 c;

    c[0] = _doublesub(a[0],b[0]);
    c[1] = _doublesub(a[1],b[1]);
    c[2] = _doublesub(a[2],b[2]);
    c[3] = _doublesub(a[3],b[3]);
    return c;
}

double4 _4xdoublemul(double4 a, double4 b) {
    double4 c;

    c[0] = _doublemul(a[0],b[0]);
    c[1] = _doublemul(a[1],b[1]);
    c[2] = _doublemul(a[2],b[2]);
    c[3] = _doublemul(a[3],b[3]);
    return c;
}

double4 _4xdoublediv(double4 a, double4 b) {
    double4 c;

    c[0] = _doublediv(a[0],b[0]);
    c[1] = _doublediv(a[1],b[1]);
    c[2] = _doublediv(a[2],b[2]);
    c[3] = _doublediv(a[3],b[3]);
    return c;
}

float2 _2xfloatadd(float2 a, float2 b) {
    float2 c;

    c[0] = _floatadd(a[0],b[0]);
    c[1] = _floatadd(a[1],b[1]);
    return c;
}

float2 _2xfloatsub(float2 a, float2 b) {
    float2 c;

    c[0] = _floatsub(a[0],b[0]);
    c[1] = _floatsub(a[1],b[1]);
    return c;
}

float2 _2xfloatmul(float2 a, float2 b) {
    float2 c;

    c[0] = _floatmul(a[0],b[0]);
    c[1] = _floatmul(a[1],b[1]);
    return c;
}

float2 _2xfloatdiv(float2 a, float2 b) {
    float2 c;

    c[0] = _floatdiv(a[0],b[0]);
    c[1] = _floatdiv(a[1],b[1]);
    return c;
}

float4 _4xfloatadd(float4 a, float4 b) {
    float4 c;

    c[0] = _floatadd(a[0],b[0]);
    c[1] = _floatadd(a[1],b[1]);
    c[2] = _floatadd(a[2],b[2]);
    c[3] = _floatadd(a[3],b[3]);
    return c;
}

float4 _4xfloatsub(float4 a, float4 b) {
    float4 c;

    c[0] = _floatsub(a[0],b[0]);
    c[1] = _floatsub(a[1],b[1]);
    c[2] = _floatsub(a[2],b[2]);
    c[3] = _floatsub(a[3],b[3]);
    return c;
}

float4 _4xfloatmul(float4 a, float4 b) {
    float4 c;

    c[0] = _floatmul(a[0],b[0]);
    c[1] = _floatmul(a[1],b[1]);
    c[2] = _floatmul(a[2],b[2]);
    c[3] = _floatmul(a[3],b[3]);
    return c;
}

float4 _4xfloatdiv(float4 a, float4 b) {
    float4 c;

    c[0] = _floatdiv(a[0],b[0]);
    c[1] = _floatdiv(a[1],b[1]);
    c[2] = _floatdiv(a[2],b[2]);
    c[3] = _floatdiv(a[3],b[3]);
    return c;
}
