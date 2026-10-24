#pragma once
#include <string>
#include <utility>
#include "ast.h"

ExprPtr parse(const std::string& src);

std::pair<int, int> line_col(const std::string& src, int pos);