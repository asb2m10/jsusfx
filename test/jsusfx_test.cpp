/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include <string.h>
#include "../jsusfx.h"
#include <stdio.h>
#include <stdarg.h>

class JsusFxTest : public JsusFx {
public:
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf(output);
        printf("\n");
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf(output);
        printf("\n");
    }
};


void test_script(char *path) {
	JsusFxTest *fx;
    float *in[2];
    float *out[2];
    
    in[0] = new float[64];
    in[1] = new float[64];
    
    out[0] = new float[64];
    out[1] = new float[64];
        
    fx = new JsusFxTest();
	
	std::ifstream is(path);
    
	printf("compile %d: %s\n", fx->compile(is), path);
    
    fx->prepare(44100, 64);
    fx->process(in, out, 64);
    fx->dumpvars();
	delete fx;
}


int main(int argc, char *argv[]) {
	
	JsusFx::init();
    
    //getchar();
    test_script("/home/asb2m10/src/jsusfx/scripts/liteon/np1136peaklimiter");

    test_script("/home/asb2m10/src/jsusfx/pd/gain.jsfx");	
	return 0;
}
