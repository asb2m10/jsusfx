/**
 * This code is used to test Linux native x86 EEL2 support and why it requires execstack 
 */


#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
int main(int argc, char **argv) {
    void *handle;
    void (*test_jsfx)(void);
    char *error;
    handle = dlopen ("./jsusfx_test.so", RTLD_LAZY);
    if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }
    dlerror();  
    test_jsfx = dlsym(handle, "test_jsfx");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
    (*test_jsfx)();
    dlclose(handle);
    return 0;
}
