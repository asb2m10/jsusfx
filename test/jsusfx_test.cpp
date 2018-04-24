/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include <string.h>
#include "../jsusfx.h"
#include <stdio.h>
#include <stdarg.h>

#define ENABLE_INOUT_TEST 1

#define TEST_GFX 1

#if ENABLE_INOUT_TEST
	#include <math.h>
#endif

#if TEST_GFX
	#include "../jsusfx_gfx.h"
#endif



struct JsusFxPathLibraryTest : JsusFxPathLibrary {
	static bool fileExists(const std::string &filename) {
		std::ifstream is(filename);
		return is.is_open();
	}

	virtual bool resolveImportPath(const std::string &importPath, const std::string &parentPath, std::string &resolvedPath) override {
		const size_t pos = parentPath.rfind('/', '\\');
		if (pos != std::string::npos)
			resolvedPath = parentPath.substr(0, pos + 1);
		resolvedPath += importPath;
		return fileExists(resolvedPath);
	}
	
	virtual std::istream* open(const std::string &path) override {
		std::ifstream *stream = new std::ifstream(path);
		if ( stream->is_open() == false ) {
			delete stream;
			stream = nullptr;
		}
		
		return stream;
	}
	
	virtual void close(std::istream *&stream) override {
		delete stream;
		stream = nullptr;
	}
};

class JsusFxTest : public JsusFx {
public:
	JsusFxTest(JsusFxPathLibrary &pathLibrary)
		: JsusFx(pathLibrary) {
	}
	
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
	
	JsusFxPathLibraryTest pathLibrary;
	
    fx = new JsusFxTest(pathLibrary);
	
#if TEST_GFX
	JsusFxGfx_Log gfx;
	fx->gfx = &gfx;
	gfx.init(fx->m_vm);
#endif
	
	printf("compile %d: %s\n", fx->compile(pathLibrary, path), path);
	
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
	fx->process(in, out, 64, 2, 2);

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
		
	#if TEST_GFX
        fx->draw();
	#endif
		
	delete fx;
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
    if (argc >= 2) {
        const char *path = argv[1];
        JsusFx::init();
        test_script(path);
    } else {
        test_jsfx();	
    }
	return 0;
}
