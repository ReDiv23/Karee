#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

struct Span {
    int start = 0;
    int end = 0;
};

enum class Kind { Num, Bool, Var, Lam, App, Let, LetRec, If, BinOp, List, Map, Fold, RecRef,Neg };

struct Expr;
using ExprPtr = std::shared_ptr<const Expr>;
using Path = std::vector<int>;

// kids layout per kind:
//   Num/Bool/Var : (none)         value in num / boolean / name
//   Lam          : [body]         name = parameter
//   App          : [fn, arg]
//   Let, LetRec  : [value, body]  name = binder
//   If           : [cond, then, else]
//   BinOp        : [lhs, rhs]     name = operator ("+", "==", ...)
//   List         : [elem, elem, ...]
//   Map          : [f, list]
//   Fold         : [f, acc, list]
//   RecRef       : [lam]          name = the recursive name. Never written by the user: created by `let rec`, prints as just the name.
//   Neg          : [operand]      unary minus, e.g. -x  (a plain -3 is just a Num)
struct Expr {
    Kind kind = Kind::Num;
    Span span;
    long long num = 0;
    bool boolean = false;
    std::string name;
    std::vector<ExprPtr> kids;
};

inline ExprPtr mk_node(Kind k, Span sp, std::vector<ExprPtr> kids, std::string name = "", long long num = 0, bool b = false) {
    auto e = std::make_shared<Expr>();

    e->kind = k;
    e->span = sp;
    e->kids = std::move(kids);
    e->name = std::move(name);
    e->num = num;
    e->boolean = b;

    return e;
}

struct ParseError : std::runtime_error {
    Span span;
    ParseError(const std::string& m, Span s) : std::runtime_error(m), span(s) {}
};

inline int binop_prec(const std::string& op) {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return 3;
    if (op == "+" || op == "-") return 4;
    if (op == "*" || op == "/" || op == "%") return 5;
    if (op == "^") return 6;
    return 0;
}

inline bool is_right_assoc(const std::string& op) { return op == "^"; }