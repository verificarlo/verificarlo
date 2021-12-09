#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <climits>
#include <string>
#include <math.h>

#include <chrono>

#include <omp.h>

#define NB_ELEM 1e06
#define NB_ITER 1e03
#define NB_SAMPLES 1e03
#define NB_THREADS 1

const char* format(float x) {
    std::string fmt = "%.7f";
    const char* fmt_cstr = fmt.c_str();
    return strdup(fmt_cstr);
}

const char* format(double x) {
    std::string fmt = "%.16f";
    const char* fmt_cstr = fmt.c_str();
    return strdup(fmt_cstr);
}

using namespace std;
using namespace std::chrono;


REAL NaiveSum(const vector<REAL> &values, const vector<REAL>::iterator &start, const vector<REAL>::iterator &end)
{
    REAL tmp_sum = 0.0;
    
    for(vector<REAL>::iterator it = start; (it != end) && (it != values.end()); it++)
    {
        tmp_sum += *it;
    }
    
    return tmp_sum;
}


int main(void) {
    vector<REAL> inputs;
    vector<REAL> res_tmp;
    REAL sum;
    REAL x;
    const char *flt_fmt = format(x);
    char fmt[1024];
    int nThreads = 0;
    int workShare = (int)ceil(1.0*NB_ELEM/NB_THREADS);
    steady_clock::time_point t_begin, t_end;

    sprintf(fmt, "Result: %s\n", flt_fmt);

    // fill the input vector with NB_ELEM random numbers
    srand48((long int)0);
    for (size_t i = 0; i < NB_ELEM; i++)
    {
        // inputs.push_back((REAL)INT_MAX*(REAL)drand48());
        inputs.push_back((REAL)drand48());
        // inputs.push_back(1.0);
    }

    printf("Number of threads: %d\n", NB_THREADS);
    printf("Number of iterations: %d\n", (int)NB_ITER);
    printf("Generated %lu random numbers\n", inputs.size());

    omp_set_num_threads(NB_THREADS);

    res_tmp.resize(NB_THREADS);
    for (size_t i = 0; i < NB_THREADS; i++)
    {
        res_tmp[i] = 0;
    }

    // start measuring the time
    t_begin = steady_clock::now();

    // do a fixed numer of iterations of the summations
    for (size_t j = 0; j < NB_ITER; j++)
    {
        // do the summations in parallel
        #pragma omp parallel
        {
            #pragma omp master
            nThreads = omp_get_num_threads();

            #pragma omp for
            for (size_t i = 0; i < NB_THREADS; i++)
            {
                vector<REAL>::iterator it_start = inputs.begin()+i*workShare;
                vector<REAL>::iterator it_end = inputs.begin()+(i+1)*workShare;

                res_tmp[i] = NaiveSum(inputs, it_start, it_end);
            }
        }
        
        // wait for all partial sums to be computed
        #pragma omp barrier

        // sum the partial sums
        #pragma omp master
        {
            sum = 0.0;
            for (size_t i = 0; i < NB_THREADS; i++)
            {
                sum += res_tmp[i];
            }
        }
    }

    // stop measuring the time
    t_end = steady_clock::now();

    printf("Time elapsed: %.6fs\n", duration_cast<std::chrono::microseconds>(t_end - t_begin).count()/1e6);

    printf(fmt, sum);

    return EXIT_SUCCESS;
}