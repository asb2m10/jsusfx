/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include <string.h>
#include "../jsusfx.h"
#include <stdio.h>

/*

void JsusFx::displayMsg(const char *fmt, ...) {
    char output[4096];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(output, 4095, fmt, argptr);
    va_end(argptr);

    printf(output);
}

void JsusFx::displayError(const char *fmt, ...) {
    char output[4096];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(output, 4095, fmt, argptr);
    va_end(argptr);
    
    printf("error: %s",output);
}
*/

void test_script(char *path) {
	JsusFx *fx;
    float *in[2];
    float *out[2];
    
    in[0] = new float[64];
    in[1] = new float[64];
    
    out[0] = new float[64];
    out[1] = new float[64];
    
    
    fx = new JsusFx();
	
	std::ifstream is(path);
    
	printf("compile %d: %s\n", fx->compile(is), path);
    
    fx->prepare(44100, 64);
    fx->process(in, out, 64);
    
    fx->displayVar();

	delete fx;
}


int main(int argc, char *argv[]) {
	
	JsusFx::init();
    
    //getchar();
	
    test_script("/Users/asb2m10/Documents/src/jsusfx/samples/pd_path/chorus");
    test_script("/Users/asb2m10/Documents/src/jsusfx/samples/pd_path/flanger");
    test_script("/Users/asb2m10/Documents/src/jsusfx/samples/pd_path/limiter");
    test_script("/Users/asb2m10/Documents/src/jsusfx/samples/pd_path/distortion");
    test_script("/Users/asb2m10/Documents/src/jsusfx/samples/pd_path/scratchy");
    
	return 0;
}
