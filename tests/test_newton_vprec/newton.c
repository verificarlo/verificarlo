#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <interflop/interflop.h>

#define PI 3.141592653589793238462643383279502884
#define TARGET_INVERSE_PI (1.0 / PI)
#define MAX_ITER 20

/* 
 * Newton-Raphson method for finding 1/PI (root of f(x) = 1/x - PI = 0)
 * Initial guess x0 = 0.03661977236758133 (yields x_1 = 0.0690266447076745)
 * Args:
 *   argv[1] = Mode (0 = Standard IEEE-754 binary64, 1 = Dynamic VPREC schedule, 2 = Mixed precision)
 *   argv[2] = Tolerance exponent (e.g. 5 for 1e-5, 10 for 1e-10, 15 for 1e-15, default: 15)
 */
int main(int argc, char **argv) {
    int mode = (argc > 1) ? atoi(argv[1]) : 0;
    int tol_exp = (argc > 2) ? atoi(argv[2]) : 15;
    double tol = pow(10.0, -tol_exp);

    int prec_schedule[10] = {3, 2, 3, 1, 2, 9, 12, 27, 51, 50};

    if (mode == 1) {
        const char *sched_file = getenv("VFC_SCHEDULE_FILE");
        if (!sched_file) sched_file = "vfc_schedule.txt";
        FILE *f = fopen(sched_file, "r");
        if (f) {
            for (int i = 0; i < 10; i++) {
                if (fscanf(f, "%d", &prec_schedule[i]) != 1) break;
            }
            fclose(f);
        }
    }

    double x_k, x_k1 = 0.03661977236758133;
    double rel_err = 1.0;
    int k = 0;

    printf("# k\tx_k1\t\t\trel_err\t\ts10\ts2\n");

    do {
        x_k = x_k1;

        if (mode == 1 && k < 10) {
            /* Set VPREC significand precision for iteration k */
            interflop_call(INTERFLOP_SET_PRECISION_BINARY64, prec_schedule[k]);
        } else if (mode == 2) {
            /* Mixed precision: first 7 iterations (k = 0..6) binary32 (24 significand bits) */
            if (k < 7) {
                interflop_call(INTERFLOP_SET_PRECISION_BINARY64, 24);
            } else {
                interflop_call(INTERFLOP_SET_PRECISION_BINARY64, 53);
            }
        }

        /* Newton-Raphson iteration: x_{k+1} = x_k * (2 - PI * x_k)
         * Floating-point operations are intercepted and quantized by VPREC backend in C */
        x_k1 = x_k * (2.0 - PI * x_k);

        rel_err = fabs((TARGET_INVERSE_PI - x_k1) / TARGET_INVERSE_PI);
        double s10 = (rel_err == 0.0) ? 15.6 : -log10(rel_err);
        double s2 = (rel_err == 0.0) ? 51.8 : -log2(rel_err);

        printf("%d\t%.16f\t%.6e\t%.2f\t%.2f\n", k, x_k1, rel_err, s10, s2);
        k++;
    } while (k < 10 && k < MAX_ITER);

    /* Return 0 if converged within target tolerance tol in 10 iterations, else 1 */
    return (rel_err < tol) ? 0 : 1;
}
