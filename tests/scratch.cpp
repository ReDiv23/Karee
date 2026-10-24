#include <iostream>
#include "ast.h"

int main() {
    // build the tree for  1 + 2  by hand
    ExprPtr one = mk_node(Kind::Num, {0, 1}, {}, "", 1);
    ExprPtr two = mk_node(Kind::Num, {4, 5}, {}, "", 2);
    ExprPtr sum = mk_node(Kind::BinOp, {0, 5}, {one, two}, "+");

    std::cout << sum->name << " has " << sum->kids.size() << " kids; left = " << sum->kids[0]->num << " right = " << sum->kids[1]->num << "\n";
}