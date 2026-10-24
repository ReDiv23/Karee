#include <iostream>
#include "parser.h"

std::string dump(const ExprPtr& e) {
    switch (e->kind) {
        case Kind::Num:  return std::to_string(e->num);
        case Kind::Bool: return e->boolean ? "true" : "false";
        case Kind::Var:  return e->name;
        default: break;
    }
    static const char* names[] = {"Num", "Bool", "Var", "Lam", "App", "Let",
                                  "LetRec", "If", "BinOp", "List", "Map", "Fold"};
    std::string s = "(" + std::string(names[(int)e->kind]);
    if (!e->name.empty()) s += ":" + e->name;
    for (const auto& k : e->kids) s += " " + dump(k);
    return s + ")";
}

int main() {
    const char* tests[] = {
        "let x = 2 in x + 1",
        "\\x -> x * 2",
        "\\x y -> x + y",
        "if a then 1 else 2",
        "let rec f = \\n -> if n == 0 then 1 else n * f (n - 1) in f 4",
        "map (\\x -> x + 1) [1, 2]",
        "1 + \\x -> x",
        "let x = in 3",
        "if 1 then 2"};
    for (const char* src : tests) {
        std::cout << src << "   =>   ";
        try {
            std::cout << dump(parse(src)) << "\n";
        } catch (const ParseError& e) {
            std::cout << "ERROR: " << e.what() << " at " << e.span.start << "\n";
        }
    }
}