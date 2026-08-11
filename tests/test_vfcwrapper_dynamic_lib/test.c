#include <stdio.h>
#include <stdlib.h>

double add_func(double a, double b) {
    return a + b;
}

int main(void) {
    double x = 1.5;
    double y = 2.5;
    double res = add_func(x, y);
    printf("Result: %f\n", res);
    if (res != 4.0) {
        return 1;
    }
    return 0;
}
