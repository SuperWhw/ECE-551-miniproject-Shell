#include <iostream>
#include <stdlib.h>

int main(int argc, char ** argv) {
	for(int i = 0; i < argc; ++i) {
		std::cout << argv[i] << '\n';
	}
	return EXIT_SUCCESS;
}
