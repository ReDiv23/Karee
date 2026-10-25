#include "printer.h"

namespace {

// `ctx` says how tightly the surrounding position binds:
//   0     top level / free position
//   1..6  operand of a binary operator with that precedence
//   7     function position in an application   (f in  f x)
//   8     argument position                     (x in  f x)
class Printer {
public:
    explicit Printer(const Path* target) : target_(target) {}
    std::string out;
    Span redex{-1, -1};

    void go(const ExprPtr& e, int ctx) {
        bool paren = needs_paren(*e, ctx);
        if (paren) out += '(';
        int start = (int)out.size();

        switch (e->kind) {
            case Kind::Num:
                out += std::to_string(e->num);
                break;
            case Kind::Bool:
                out += e->boolean ? "true" : "false";
                break;
            case Kind::Var:
                out += e->name;
                break;
            case Kind::Lam:
                out += "\\" + e->name + " -> ";
                child(e->kids[0], 0, 0);
                break;
            case Kind::App:
                child(e->kids[0], 0, 7);
                out += ' ';
                child(e->kids[1], 1, 8);
                break;
            case Kind::Let:
            case Kind::LetRec:
                out += e->kind == Kind::Let ? "let " : "let rec ";
                out += e->name + " = ";
                child(e->kids[0], 0, 0);
                out += " in ";
                child(e->kids[1], 1, 0);
                break;
            case Kind::If:
                out += "if ";
                child(e->kids[0], 0, 0);
                out += " then ";
                child(e->kids[1], 1, 0);
                out += " else ";
                child(e->kids[2], 2, 0);
                break;
            case Kind::BinOp: {
                int p = binop_prec(e->name);
                bool right = is_right_assoc(e->name);
                child(e->kids[0], 0, right ? p + 1 : p);
                out += " " + e->name + " ";
                child(e->kids[1], 1, right ? p : p + 1);
                break;
            }
            case Kind::List:
                out += '[';
                for (size_t i = 0; i < e->kids.size(); i++) {
                    if (i) out += ", ";
                    child(e->kids[i], (int)i, 0);
                }
                out += ']';
                break;
            case Kind::Map:
                out += "map ";
                child(e->kids[0], 0, 8);
                out += ' ';
                child(e->kids[1], 1, 8);
                break;
            case Kind::Fold:
                out += "fold ";
                child(e->kids[0], 0, 8);
                out += ' ';
                child(e->kids[1], 1, 8);
                out += ' ';
                child(e->kids[2], 2, 8);
                break;
            case Kind::RecRef:
                out += e->name;
                break;
        }

        int end = (int)out.size();
        if (target_ && cur_ == *target_) redex = {start, end};
        if (paren) out += ')';
    }

private:
    const Path* target_;
    Path cur_;

    void child(const ExprPtr& e, int idx, int ctx) {
        cur_.push_back(idx);
        go(e, ctx);
        cur_.pop_back();
    }

    static bool needs_paren(const Expr& e, int ctx) {
        switch (e.kind) {
            case Kind::Num:
                return e.num < 0 && ctx > 0;
            case Kind::Lam:
            case Kind::Let:
            case Kind::LetRec:
            case Kind::If:
                return ctx > 0;
            case Kind::BinOp:
                return binop_prec(e.name) < ctx;
            case Kind::App:
            case Kind::Map:
            case Kind::Fold:
                return ctx > 7;
            default:
                return false;
        }
    }
};

}

Printed pretty(const ExprPtr& e, const Path* redex_path) {
    Printer p(redex_path);
    p.go(e, 0);
    return {p.out, p.redex};
}