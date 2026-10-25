#include <climits>
#include <algorithm>

#include "eval.h"
#include "printer.h"

bool is_value(const ExprPtr& e) {
    switch (e->kind) {
        case Kind::Num:
        case Kind::Bool:
        case Kind::Lam:
            return true;
        case Kind::List:
            for (const auto& k : e->kids) {
                if (!is_value(k)) return false;
            }
            return true;
        default:
            return false;
    }
}

static ExprPtr with_kids(const ExprPtr& e, std::vector<ExprPtr> kids) {
    auto n = std::make_shared<Expr>(*e);
    n->kids = std::move(kids);
    return n;
}

ExprPtr subst(const ExprPtr& e, const std::string& x, const ExprPtr& v) {
    switch (e->kind) {
        case Kind::Var:
            return e->name == x ? v : e;
        case Kind::Lam:
        case Kind::LetRec:
        case Kind::RecRef:
            if (e->name == x) return e;
            break;
        default:
            break;
    }
    std::vector<ExprPtr> kids = e->kids;
    bool changed = false;
    for (size_t i = 0; i < kids.size(); i++) {
        if (e->kind == Kind::Let && i == 1 && e->name == x) continue;
        ExprPtr k = subst(kids[i], x, v);
        if (k != kids[i]) {
            kids[i] = k;
            changed = true;
        }
    }
    return changed ? with_kids(e, kids) : e;
}

namespace {

struct Ctx {
    Path path;
    Path redex;
    std::string rule, caption;
};

Path sub(Path p, int i) {
    p.push_back(i);
    return p;
}
std::string show(const ExprPtr& e) { return pretty(e).text; }
ExprPtr num(long long n) { return mk_node(Kind::Num, {}, {}, "", n); }
ExprPtr boolean(bool b) { return mk_node(Kind::Bool, {}, {}, "", 0, b); }
void found(Ctx& cx, const char* rule, std::string caption) {
    cx.redex = cx.path;
    cx.rule = rule;
    cx.caption = std::move(caption);
}
std::string kind_name(const ExprPtr& e) {
    switch (e->kind) {
        case Kind::Num:
            return "number";
        case Kind::Bool:
            return "boolean";
        case Kind::List:
            return "list";
        default:
            return "value";
    }
}

ExprPtr rec(const ExprPtr& e, Ctx& cx);

ExprPtr step_child(const ExprPtr& e, size_t i, Ctx& cx) {
    cx.path.push_back((int)i);
    ExprPtr nk = rec(e->kids[i], cx);
    cx.path.pop_back();
    std::vector<ExprPtr> kids = e->kids;
    kids[i] = nk;
    return with_kids(e, kids);
}

bool ipow(long long base, long long exp, long long& out) {
    long long result = 1;
    while (exp > 0) {
        if ((exp & 1) && __builtin_mul_overflow(result, base, &result)) return false;
        exp >>= 1;
        if (exp > 0 && __builtin_mul_overflow(base, base, &base)) return false;
    }
    out = result;
    return true;
}

ExprPtr binop(const ExprPtr& e, Ctx& cx) {
    const ExprPtr& l = e->kids[0];
    const ExprPtr& r = e->kids[1];
    const std::string& op = e->name;

    if (!is_value(l)) return step_child(e, 0, cx);

    if (op == "&&" || op == "||") {
        if (l->kind != Kind::Bool)
            throw RuntimeError("left side of '" + op + "' must be true or false", sub(cx.path, 0));
        bool is_and = (op == "&&");
        if (l->boolean == !is_and) {
            found(cx, "short-circuit", "'" + op + "' already knows the answer: " + show(l));
            return l;
        }
        found(cx, "and-or", "Left side doesn't decide it, so the result is the right side");
        return r;
    }

    if (!is_value(r)) return step_child(e, 1, cx);

    if (op == ":") {
        if (r->kind != Kind::List) throw RuntimeError("right side of ':' must be a list", sub(cx.path, 1));

        std::vector<ExprPtr> items;
        items.push_back(l);
        items.insert(items.end(), r->kids.begin(), r->kids.end());

        ExprPtr res = mk_node(Kind::List, e->span, items);
        found(cx, "list-cons", "Prepend " + show(l) + " to the list");
        return res;
    }

    if (op == "++") {
        if (l->kind != Kind::List || r->kind != Kind::List) throw RuntimeError("'++' needs two lists", cx.path);

        std::vector<ExprPtr> items = l->kids;
        items.insert(items.end(), r->kids.begin(), r->kids.end());

        ExprPtr res = mk_node(Kind::List, e->span, items);
        found(cx, "list-append", "Concatenate the two lists");
        return res;
    }

    ExprPtr res;
    if (op == "==" || op == "!=") {
        bool eq = false;
        if (l->kind == Kind::Num && r->kind == Kind::Num) eq = (l->num == r->num);
        else if (l->kind == Kind::Bool && r->kind == Kind::Bool) eq = (l->boolean == r->boolean);
        else throw RuntimeError("'" + op + "' compares two numbers or two booleans", cx.path);
        res = boolean(op == "==" ? eq : !eq);
    }
    else {
        if (l->kind != Kind::Num) throw RuntimeError("'" + op + "' needs a number, got a " + kind_name(l), sub(cx.path, 0));
        if (r->kind != Kind::Num) throw RuntimeError("'" + op + "' needs a number, got a " + kind_name(r), sub(cx.path, 1));
        long long a = l->num, b = r->num, out = 0;
        if (op == "+") {
            if (__builtin_add_overflow(a, b, &out)) throw RuntimeError("integer overflow", cx.path);
            res = num(out);
        }
        else if (op == "-") {
            if (__builtin_sub_overflow(a, b, &out)) throw RuntimeError("integer overflow", cx.path);
            res = num(out);
        }
        else if (op == "*") {
            if (__builtin_mul_overflow(a, b, &out)) throw RuntimeError("integer overflow", cx.path);
            res = num(out);
        }
        else if (op == "/") {
            if (b == 0) throw RuntimeError("division by zero", sub(cx.path, 1));
            if (a == LLONG_MIN && b == -1) throw RuntimeError("integer overflow", cx.path);
            res = num(a / b);
        }
        else if (op == "%") {
            if (b == 0) throw RuntimeError("modulo by zero", sub(cx.path, 1));
            long long m = (b == -1) ? 0 : a % b;
            if (m != 0 && ((m < 0) != (b < 0))) m += b;
            res = num(m);
        }
        else if (op == "^") {
            if (b < 0) throw RuntimeError("negative exponent: '^' works on whole numbers", sub(cx.path, 1));
            if (!ipow(a, b, out)) throw RuntimeError("integer overflow", cx.path);
            res = num(out);
        }
        else if (op == "<") res = boolean(a < b);
        else if (op == ">") res = boolean(a > b);
        else if (op == "<=") res = boolean(a <= b);
        else if (op == ">=") res = boolean(a >= b);
    }
    found(cx, "arith", "Compute " + show(e) + " = " + show(res));
    return res;
}

bool is_list_builtin(const ExprPtr& e) {
    if (e->kind != Kind::Var) return false;

    return e->name == "head" ||
           e->name == "tail" ||
           e->name == "isEmpty" ||
           e->name == "length" ||
           e->name == "not";
}

ExprPtr rec(const ExprPtr& e, Ctx& cx) {
    switch (e->kind) {
        case Kind::Var:
            throw RuntimeError("unbound variable '" + e->name + "'", cx.path);

        case Kind::List:
            for (size_t i = 0; i < e->kids.size(); i++) {
                if (!is_value(e->kids[i])) return step_child(e, i, cx);
            }
            return e;

        case Kind::BinOp:
            return binop(e, cx);

        case Kind::Range: {
            const ExprPtr& start = e->kids[0];
            const ExprPtr& end = e->kids[1];

            if (!is_value(start)) return step_child(e, 0, cx);

            if (!is_value(end)) return step_child(e, 1, cx);

            if (start->kind != Kind::Num) throw RuntimeError("range start must be a number", sub(cx.path, 0));

            if (end->kind != Kind::Num) throw RuntimeError("range end must be a number", sub(cx.path, 1));

            std::vector<ExprPtr> items;

            if (start->num <= end->num) {
                long long n = start->num;

                while (true) {
                    items.push_back(num(n));

                    if (n == end->num) break;

                    if (n == LLONG_MAX) throw RuntimeError("range integer overflow", cx.path);

                    ++n;

                    if (items.size() > 10000) throw RuntimeError("range exceeds 10000 elements", cx.path);
                }
            }

            ExprPtr result = mk_node(Kind::List, e->span, items);

            found(cx, "range", "Expand the inclusive integer range");
            return result;
        }

        case Kind::If: {
            const ExprPtr& c = e->kids[0];
            if (!is_value(c)) return step_child(e, 0, cx);
            if (c->kind != Kind::Bool) throw RuntimeError("the condition of 'if' must be true or false", sub(cx.path, 0));
            found(cx, c->boolean ? "if-true" : "if-false", c->boolean ? "Condition is true: keep the 'then' branch" : "Condition is false: keep the 'else' branch");
            return e->kids[c->boolean ? 1 : 2];
        }

        case Kind::Let: {
            const ExprPtr& v = e->kids[0];
            if (!is_value(v)) return step_child(e, 0, cx);
            found(cx, "let", "Bind " + e->name + " = " + show(v) + " in the body");
            return subst(e->kids[1], e->name, v);
        }

        case Kind::App: {
            const ExprPtr& f = e->kids[0];
            const ExprPtr& a = e->kids[1];
            if (is_list_builtin(f)) {
                if (!is_value(a)) return step_child(e, 1, cx);

                const std::string& name = f->name;

                if (name == "not") {
                    if (a->kind != Kind::Bool) throw RuntimeError("not needs a boolean", sub(cx.path, 1));

                    found(cx, "not", "Negate the boolean");
                    return boolean(!a->boolean);
                }

                if (a->kind != Kind::List) throw RuntimeError(name + " needs a list", sub(cx.path, 1));

                if (name == "isEmpty") {
                    found(cx, "list-empty", "Check whether the list is empty");
                    return boolean(a->kids.empty());
                }

                if (name == "length") {
                    found(cx, "list-length", "Count the list elements");
                    return num(static_cast<long long>(a->kids.size()));
                }

                if (a->kids.empty()) throw RuntimeError(name + " cannot be applied to an empty list", sub(cx.path, 1));

                if (name == "head") {
                    found(cx, "list-head", "Take the first element");
                    return a->kids.front();
                }

                if (name == "tail") {
                    std::vector<ExprPtr> rest(a->kids.begin() + 1, a->kids.end());
                    found(cx, "list-tail", "Remove the first element");
                    return mk_node(Kind::List, a->span, rest);
                }
            }
            if (!is_value(f)) return step_child(e, 0, cx);
            if (!is_value(a)) return step_child(e, 1, cx);
            if (f->kind != Kind::Lam) throw RuntimeError("cannot call a " + kind_name(f) + " as a function", sub(cx.path, 0));
            found(cx, "beta", "Call the function: " + f->name + " = " + show(a));
            return subst(f->kids[0], f->name, a);
        }

        case Kind::Neg: {
            const ExprPtr& v = e->kids[0];
            if (!is_value(v)) return step_child(e, 0, cx);
            if (v->kind != Kind::Num) throw RuntimeError("'-' needs a number, got a " + kind_name(v), sub(cx.path, 0));
            long long out = 0;
            if (__builtin_sub_overflow(0LL, v->num, &out)) throw RuntimeError("integer overflow", cx.path);
            ExprPtr res = num(out);
            found(cx, "arith", "Compute " + show(e) + " = " + show(res));
            return res;
        }

        case Kind::LetRec: {
            const ExprPtr& lam = e->kids[0];
            if (lam->kind != Kind::Lam) throw RuntimeError("'let rec' needs a function (\\x -> ...) on the right-hand side", sub(cx.path, 0));
            ExprPtr ref = mk_node(Kind::RecRef, {}, {lam}, e->name);   // "f, as its own definition"
            ExprPtr unrolled = subst(lam, e->name, ref);               // recursive calls become refs
            found(cx, "letrec", "Unroll: " + e->name + " stands for its own definition");
            return subst(e->kids[1], e->name, unrolled);
        }

        case Kind::RecRef:
            found(cx, "unroll", "Unfold " + e->name + " into its definition");
            return subst(e->kids[0], e->name, e);

        case Kind::Map: {
            const ExprPtr& f = e->kids[0];
            const ExprPtr& xs = e->kids[1];
            if (!is_value(f))  return step_child(e, 0, cx);
            if (!is_value(xs)) return step_child(e, 1, cx);
            if (f->kind != Kind::Lam) throw RuntimeError("map needs a function as its first argument", sub(cx.path, 0));
            if (xs->kind != Kind::List) throw RuntimeError("map needs a list as its second argument", sub(cx.path, 1));
            std::vector<ExprPtr> calls;                          // [f v1, f v2, ...]
            for (const auto& v : xs->kids) calls.push_back(mk_node(Kind::App, {}, {f, v}));
            found(cx, "map-expand", "Apply the function to every element of the list");
            return mk_node(Kind::List, {}, calls);
        }

        case Kind::Fold: {
            for (size_t i = 0; i < 3; i++) {
                if (!is_value(e->kids[i])) return step_child(e, i, cx);
            }
            const ExprPtr& f = e->kids[0];
            const ExprPtr& acc = e->kids[1];
            const ExprPtr& xs = e->kids[2];
            if (f->kind != Kind::Lam) throw RuntimeError("fold needs a function as its first argument", sub(cx.path, 0));
            if (xs->kind != Kind::List) throw RuntimeError("fold needs a list as its third argument", sub(cx.path, 2));
            if (xs->kids.empty()) {
                found(cx, "fold-done", "The list is empty: the result is the accumulator");
                return acc;
            }
            ExprPtr head = xs->kids[0];
            ExprPtr rest = mk_node(Kind::List, {}, std::vector<ExprPtr>(xs->kids.begin() + 1, xs->kids.end()));
            ExprPtr new_acc = mk_node(Kind::App, {}, {mk_node(Kind::App, {}, {f, acc}), head});
            found(cx, "fold-step", "Feed " + show(head) + " into the accumulator");
            return mk_node(Kind::Fold, {}, {f, new_acc, rest});
        }

        case Kind::Filter: {
            if (e->name != "$state") {
                const ExprPtr& predicate = e->kids[0];
                const ExprPtr& xs = e->kids[1];

                if (!is_value(predicate)) return step_child(e, 0, cx);

                if (!is_value(xs)) return step_child(e, 1, cx);

                if (predicate->kind != Kind::Lam) throw RuntimeError("filter needs a function as its first argument", sub(cx.path, 0));

                if (xs->kind != Kind::List) throw RuntimeError("filter needs a list as its second argument", sub(cx.path, 1));

                ExprPtr empty = mk_node(Kind::List, {}, {});

                ExprPtr call = xs->kids.empty() ? boolean(true) : mk_node(Kind::App, {}, {predicate, xs->kids[0]});

                found(cx, "filter-start", "Start testing the list elements");

                return mk_node(Kind::Filter, e->span, {predicate, xs, empty, xs, empty, call}, "$state");
            }

            const ExprPtr& predicate = e->kids[0];
            const ExprPtr& remaining = e->kids[1];
            const ExprPtr& output = e->kids[2];
            const ExprPtr& original = e->kids[3];
            const ExprPtr& decisions = e->kids[4];
            const ExprPtr& call = e->kids[5];

            if (remaining->kids.empty()) {
                found(cx, "filter-done", "All elements have been tested");
                return output;
            }

            if (!is_value(call)) return step_child(e, 5, cx);

            if (call->kind != Kind::Bool) throw RuntimeError("filter predicate must return true or false", sub(cx.path, 5));

            const ExprPtr& current = remaining->kids[0];

            std::vector<ExprPtr> rest(remaining->kids.begin() + 1, remaining->kids.end());

            ExprPtr nextRemaining = mk_node(Kind::List, remaining->span, rest);

            std::vector<ExprPtr> nextOutput = output->kids;
            std::vector<ExprPtr> nextDecisions = decisions->kids;

            nextDecisions.push_back(call);

            if (call->boolean) nextOutput.push_back(current);

            ExprPtr newOutput = mk_node(Kind::List, output->span, nextOutput);

            ExprPtr newDecisions = mk_node(Kind::List, decisions->span, nextDecisions);

            ExprPtr nextCall = nextRemaining->kids.empty() ? boolean(true) : mk_node(Kind::App, {}, {predicate, nextRemaining->kids[0]});

            found(cx, call->boolean ? "filter-keep" : "filter-drop", call->boolean ? "Keep " + show(current) + ": predicate is true" : "Drop " + show(current) + ": predicate is false");

            return mk_node(Kind::Filter, e->span, {predicate, nextRemaining, newOutput, original, newDecisions, nextCall}, "$state");
        }

        case Kind::Zip: {
            const ExprPtr& xs = e->kids[0];
            const ExprPtr& ys = e->kids[1];

            if (!is_value(xs)) return step_child(e, 0, cx);

            if (!is_value(ys)) return step_child(e, 1, cx);

            if (xs->kind != Kind::List) throw RuntimeError("zip needs a list as its first argument", sub(cx.path, 0));

            if (ys->kind != Kind::List) throw RuntimeError("zip needs a list as its second argument", sub(cx.path, 1));

            std::vector<ExprPtr> pairs;

            const size_t count = std::min(xs->kids.size(), ys->kids.size());

            for (size_t i = 0; i < count; ++i) {
                pairs.push_back(mk_node(Kind::List, {}, {xs->kids[i], ys->kids[i]}));
            }

            found(cx, "zip", "Pair corresponding elements of both lists");

            return mk_node(Kind::List, e->span, pairs);
        }

        default:
            return e;
    }
}

}

std::optional<Step> step(const ExprPtr& e) {
    if (is_value(e)) return std::nullopt;
    Ctx cx;
    ExprPtr next = rec(e, cx);
    return Step{next, cx.redex, cx.rule, cx.caption};
}