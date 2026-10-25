#pragma once
#include <string>
#include "ast.h"

std::string tree_json(const ExprPtr& root, const Path* redex);