#include <emscripten.h>
#include <string>
#include "api.h"

extern "C" EMSCRIPTEN_KEEPALIVE const char* run(const char* src) {
    static std::string out;
    out = run_program(src);
    return out.c_str();
}