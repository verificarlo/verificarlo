// Dummy test to make sure that the vfc_probes system works properly

#include <stdio.h>
#include "vfc_probes.h"

int main(void) {

    vfc_probes probes = vfc_init_probes();

    float res = 1000.0f;
    float inc = 0.0001f;

    for(int i=0; i<100; i++) {
        res = res + inc;
    }


    vfc_probe(&probes, "dummy_test", "sum", res);
    vfc_dump_probes(&probes);

    return 0;
}
