/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include <string.h>
#include "../jsusfx.h"
#include <stdio.h>
#include <stdarg.h>

#define TEST_GFX 1

#if TEST_GFX
	#include "../jsusfx_gfx.h"
#endif

class JsusFxTest : public JsusFx {
public:
    void displayMsg(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf("%s", output);
        printf("\n");
    }

    void displayError(const char *fmt, ...) {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        printf("%s", output);
        printf("\n");
    }
};


void test_script(const char *path) {
	JsusFxTest *fx;
    float *in[2];
    float *out[2];
    
    in[0] = new float[64];
    in[1] = new float[64];
    
    out[0] = new float[64];
    out[1] = new float[64];
        
    fx = new JsusFxTest();
	
#if TEST_GFX
	JsusFxGfx_Log gfx;
	fx->gfx = &gfx;
	gfx.init();
#endif
	
	std::ifstream is(path);
    
    if (!is.is_open()) {
        printf("failed to open jsfx file (%s)\n", path);
    } else {
    	printf("compile %d: %s\n", fx->compile(is), path);
        
        fx->prepare(44100, 64);
        fx->process(in, out, 64);
        fx->dumpvars();
		
	#if TEST_GFX
        fx->draw();
	#endif
		
    	delete fx;
    }
}
extern "C" void test_jsfx();

void test_jsfx() {
    JsusFx::init();
#if TEST_GFX
    test_script("../scripts/liteon/3bandpeakfilter");
#else
    test_script("../pd/gain.jsfx");
#endif
}

int main(int argc, char *argv[]) {
    test_jsfx();	
	return 0;
}
