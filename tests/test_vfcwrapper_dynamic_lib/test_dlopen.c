#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*interflop_call_fn)(int id, ...);

int main(void) {
    void *handle = dlopen("libinterflop_vfcwrapper.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    interflop_call_fn call_fn = (interflop_call_fn)dlsym(handle, "interflop_call");
    if (!call_fn) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    printf("Successfully dlopened libinterflop_vfcwrapper.so and loaded interflop_call!\n");
    dlclose(handle);
    return 0;
}
