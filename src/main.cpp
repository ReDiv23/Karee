#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include "api.h"
#include "eval.h"
#include "parser.h"
#include "printer.h"

static void print_trace(const std::string& src) {
    try {
        ExprPtr cur = parse(src);
        for (int n = 1; n <= 300; n++) {
            auto s = step(cur);
            if (!s) {
                std::cout << "  = " << pretty(cur).text << "\n";
                return;
            }
            Printed p = pretty(cur, &s->redex);
            std::cout << std::setw(3) << n << ". " << p.text.substr(0, p.redex.start) << "«" << p.text.substr(p.redex.start, p.redex.end - p.redex.start) << "»" << p.text.substr(p.redex.end) << "    (" << s->rule << ")\n";
            cur = s->next;
        }
        std::cout << "  ... stopped after 300 steps\n";
    }
    catch (const ParseError& e) {
        std::cout << "parse error: " << e.what() << "\n";
    }
    catch (const RuntimeError& e) {
        std::cout << "runtime error: " << e.what() << "\n";
    }
}

int main(int argc, char** argv) {
    bool text_mode = argc > 1 && std::string(argv[1]) == "-t";
    int file_arg = text_mode ? 2 : 1;
    std::string src;
    if (argc > file_arg) {
        std::ifstream f(argv[file_arg]);
        if (!f) {
            std::cerr << "cannot open " << argv[file_arg] << "\n" << "Usage:  ./stepper [-t] <file.hs>" << "\n";
            return 1;
        }
        src.assign(std::istreambuf_iterator<char>(f), {});
    }
    else {
        src.assign(std::istreambuf_iterator<char>(std::cin), {});
    }
    if (text_mode) print_trace(src);
    else std::cout << run_program(src) << "\n";
}