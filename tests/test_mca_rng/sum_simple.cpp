#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <climits>
#include <string>

#define NB_ELEM 1e06
#define NB_SAMPLES 1e03

const char* format(float x) {
    std::string fmt = "%.7f";
    return fmt.c_str();
}

const char* format(double x) {
    std::string fmt = "%.16f";
    return fmt.c_str();
}

using namespace std;

REAL NaiveSum(const vector<REAL> &values)
{
    REAL sum = 0.0;
    
    for (REAL v : values)
    {
        sum += v;
    }
    
    return sum;
}


int main(void) {
    vector<REAL> inputs;
    REAL sum;
    REAL x;
    const char *flt_fmt = format(x);
    char fmt[1024];

    sprintf(fmt, "Result: %s\n", flt_fmt);

    // resize the input vector
    inputs.resize(NB_ELEM);

    // fill the input vector with NB_ELEM random numbers
    srand48((long int)0);
    for (int i = 0; i < NB_ELEM; i++)
    {
        inputs.push_back((REAL)INT_MAX*(REAL)drand48());
    }

    // sum the elements of the vector
    sum = NaiveSum(inputs);

    printf(fmt, sum);

    return EXIT_SUCCESS;
}