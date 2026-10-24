#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include "ast.h"

struct RuntimeError : std::runtime_error {
    Path where;
    RuntimeError(const std::string& m, Path w) : std::runtime_error(m), where(std::move(w)) {}
};

struct Step {
    ExprPtr next;
    Path redex;
    std::string rule;
    std::string caption;
};

bool is_value(const ExprPtr& e);
ExprPtr subst(const ExprPtr& e, const std::string& x, const ExprPtr& v);

std::optional<Step> step(const ExprPtr& e);