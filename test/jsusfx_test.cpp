/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include <string.h>
#include "../jsusfx.h"
#include <stdio.h>
#include <stdarg.h>

#define ENABLE_CWD_CHANGE 1
#define ENABLE_INOUT_TEST 1

#if ENABLE_CWD_CHANGE
	#include <unistd.h>
	#include <libgen.h>
#endif

#if ENABLE_INOUT_TEST
	#include <math.h>
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
	
#if ENABLE_CWD_CHANGE
	const char * bname = basename((char*)path);
	const char * dname = dirname((char*)path);
	chdir(dname);
	
	std::ifstream is(bname);
#else
	std::ifstream is(path);
#endif
	
    if (!is.is_open()) {
        printf("failed to open jsfx file (%s)\n", path);
    } else {
    	printf("compile %d: %s\n", fx->compile(is), path);
		
    	printf("desc: %s\n", fx->desc);

	#if ENABLE_INOUT_TEST
		for (int i = 0; i < 64; ++i) {
			in[0][i] = sin(i * 2.0 * M_PI / 63.0);
			in[1][i] = cos(i * 2.0 * M_PI / 63.0);
		}
		
		fx->moveSlider(1, 0.5);
        fx->moveSlider(2, 0.1);
	#endif

        fx->prepare(44100, 64);
        fx->process(in, out, 64);

	#if ENABLE_INOUT_TEST
        for (int i = 0; i < 64; ++i) {
            printf("(%.3f, %.3f) -> (%.3f, %.3f)\n",
                in[0][i],
                in[1][i],
                out[0][i],
                out[1][i]);
        }
	#endif

        fx->dumpvars();
    	delete fx;
    }
}
extern "C" void test_jsfx();

void test_jsfx() {
    JsusFx::init();
    test_script("../pd/gain.jsfx");
}

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        const char *path = argv[1];
        JsusFx::init();
        test_script(path);
    } else {
        test_jsfx();	
    }
	return 0;
}
