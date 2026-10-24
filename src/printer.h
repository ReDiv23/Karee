#pragma once
#include <string>
#include "ast.h"

struct Printed {
    std::string text;
    Span redex{-1, -1};
};

Printed pretty(const ExprPtr& e, const Path* redex_path = nullptr);