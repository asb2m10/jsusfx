/**
 * jsusfx - Opensource Jesuscript FX implementation

 */

#include <fstream>
#include "../jsusfx.h"

int main(int argc, char *argv[]) {
	JsusFx *fx;
	
	JsusFx::init();
	
	fx = new JsusFx();
	
	//std::istringstream iss("desc:super effect\n\n@init\na=10\n@sample\nspl0=a");
		
	std::ifstream is("/Users/asb2m10/Documents/src/jsusfx/example/sonic_enhancer");

	printf("compile %d", fx->compile(is));

	//printf("%g\n", *(fx->spl0));
	fx->prepare(44100, 64);
	//printf("%g\n", *(fx->spl0));

	delete fx;
	
	return 0;
}
