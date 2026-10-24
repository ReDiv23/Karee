#include <fstream>
#include <iostream>
#include <iterator>
#include "api.h"

int main(int argc, char** argv) {
    std::string src;
    if (argc > 1) {
        std::ifstream f(argv[1]);
        if (!f) {
            std::cerr << "cannot open " << argv[1] << "\n" << "Usage:  ./stepper file.hs" << "\n";
            return 1;
        }
        src.assign(std::istreambuf_iterator<char>(f), {});
    }
    else {
        src.assign(std::istreambuf_iterator<char>(std::cin), {});
    }
    std::cout << run_program(src) << "\n";
}