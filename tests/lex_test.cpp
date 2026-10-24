#include <iostream>
#include "lexer.h"

int main() {
    std::string src = "a $ b -- hi";
    for (const Token& t : lex(src)) {
        if (t.type == Tok::End) { std::cout << "<end>\n"; break; }
        std::cout << t.text << "  [" << t.span.start << "," << t.span.end << ")\n";
    }
}