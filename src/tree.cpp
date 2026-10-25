#include "tree.h"
#include <algorithm>
#include "json.h"

namespace {

const char* tag(Kind k) {
    switch (k) {
        case Kind::Num: return "num";
        case Kind::Bool: return "bool";
        case Kind::Var: return "var";
        case Kind::Lam: return "lam";
        case Kind::App: return "app";
        case Kind::Let: return "let";
        case Kind::LetRec: return "letrec";
        case Kind::If: return "if";
        case Kind::BinOp: return "op";
        case Kind::List: return "list";
        case Kind::Map: return "map";
        case Kind::Fold: return "fold";
        case Kind::RecRef: return "ref";
        case Kind::Neg: return "neg";
    }
    return "num";
}

std::string text(const Expr& e) {
    switch (e.kind) {
        case Kind::Num: return std::to_string(e.num);
        case Kind::Bool: return e.boolean ? "true" : "false";
        case Kind::Var:
        case Kind::Lam:
        case Kind::Let:
        case Kind::LetRec:
        case Kind::BinOp:
        case Kind::RecRef: return e.name;
        default: return "";
    }
}

const char* role(const Expr& p, size_t i) {
    static const char* ifr[] = {"cond", "then", "else"};
    static const char* foldr[] = {"f", "acc", "list"};
    switch (p.kind) {
        case Kind::Lam: return "body";
        case Kind::App: return i == 0 ? "fn" : "arg";
        case Kind::Let:
        case Kind::LetRec: return i == 0 ? "value" : "body";
        case Kind::If: return i < 3 ? ifr[i] : "";
        case Kind::BinOp: return i == 0 ? "lhs" : "rhs";
        case Kind::Map: return i == 0 ? "f" : "list";
        case Kind::Fold: return i < 3 ? foldr[i] : "";
        default: return "";
    }
}

void go(const ExprPtr& e, Path& cur, const Path* redex, const char* r, std::string& out) {
    bool hot = redex && cur == *redex;
    bool on = redex && !hot && cur.size() < redex->size() && std::equal(cur.begin(), cur.end(), redex->begin());

    out += std::string("{\"k\":\"") + tag(e->kind) + "\",\"t\":\"" + json_escape(text(*e)) + "\"";
    if (r[0]) out += std::string(",\"r\":\"") + r + "\"";
    if (hot) out += ",\"hot\":1";
    else if (on) out += ",\"on\":1";

    if (e->kind != Kind::RecRef && !e->kids.empty()) {
        out += ",\"c\":[";
        for (size_t i = 0; i < e->kids.size(); i++) {
            if (i) out += ",";
            cur.push_back((int)i);
            go(e->kids[i], cur, redex, role(*e, i), out);
            cur.pop_back();
        }
        out += "]";
    }
    out += "}";
}

}

std::string tree_json(const ExprPtr& root, const Path* redex) {
    std::string out;
    Path cur;
    go(root, cur, redex, "", out);
    return out;
}