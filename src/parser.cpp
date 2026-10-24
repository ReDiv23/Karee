#include "parser.h"
#include "lexer.h"

namespace {

class Parser {
public:
    explicit Parser(std::vector<Token> t) : t_(std::move(t)) {}

    ExprPtr program() {
        ExprPtr e = expr();
        if (!at(Tok::End)) fail("unexpected " + describe(peek()));
        return e;
    }

private:
    std::vector<Token> t_;
    size_t i_ = 0;

    const Token& peek() const {
        return t_[i_];
    }
    bool at(Tok k) const {
        return peek().type == k;
    }
    Token next() {
        Token k = t_[i_];
        if (k.type != Tok::End) i_++;
        return k;
    }

    static std::string describe(const Token& k) {
        return k.type == Tok::End ? "end of input" : "'" + k.text + "'";
    }
    [[noreturn]] void fail(const std::string& m) const {
        throw ParseError(m, peek().span);
    }

    Token expect(Tok k, const char* what) {
        if (!at(k)) fail(std::string("expected ") + what + ", found " + describe(peek()));
        return next();
    }

    static bool starts_atom(Tok k) {
        return k == Tok::Num || k == Tok::Ident || k == Tok::True || k == Tok::False || k == Tok::LParen || k == Tok::LBrack || k == Tok::Map || k == Tok::Fold;
    }

    ExprPtr expr() { return binop(1); }

    ExprPtr binop(int min_prec) {
        ExprPtr lhs = app();
        while (at(Tok::Op) && binop_prec(peek().text) >= min_prec) {
            Token op = next();
            int p = binop_prec(op.text);
            ExprPtr rhs = binop(p + 1);
            lhs = mk_node(Kind::BinOp, {lhs->span.start, rhs->span.end}, {lhs, rhs}, op.text);
        }
        return lhs;
    }

    ExprPtr app() {
        ExprPtr f = atom();
        while (starts_atom(peek().type)) {
            ExprPtr a = atom();
            f = mk_node(Kind::App, {f->span.start, a->span.end}, {f, a});
        }
        return f;
    }

    ExprPtr atom() {
        const Token& k = peek();
        switch (k.type) {
            case Tok::Num: {
                Token t = next();
                return mk_node(Kind::Num, t.span, {}, "", t.num);
            }
            case Tok::True: {
                Token t = next();
                return mk_node(Kind::Bool, t.span, {}, "", 0, true);
            }
            case Tok::False: {
                Token t = next();
                return mk_node(Kind::Bool, t.span, {}, "", 0, false);
            }
            case Tok::Ident: {
                Token t = next();
                return mk_node(Kind::Var, t.span, {}, t.text);
            }
            case Tok::Op:
                if (k.text == "-" && t_[i_ + 1].type == Tok::Num) {
                    Token m = next();
                    Token t = next();
                    return mk_node(Kind::Num, {m.span.start, t.span.end}, {}, "", -t.num);
                }
                break;
            case Tok::LParen: {
                next();
                ExprPtr e = expr();
                expect(Tok::RParen, "')'");
                return e;
            }
            case Tok::LBrack: {
                Token open = next();
                std::vector<ExprPtr> items;
                if (!at(Tok::RBrack)) {
                    items.push_back(expr());
                    while (at(Tok::Comma)) {
                        next();
                        items.push_back(expr());
                    }
                }
                Token close = expect(Tok::RBrack, "']'");
                return mk_node(Kind::List, {open.span.start, close.span.end}, items);
            }
            case Tok::Map: {
                Token kw = next();
                ExprPtr f = atom(), xs = atom();
                return mk_node(Kind::Map, {kw.span.start, xs->span.end}, {f, xs});
            }
            case Tok::Fold: {
                Token kw = next();
                ExprPtr f = atom(), acc = atom(), xs = atom();
                return mk_node(Kind::Fold, {kw.span.start, xs->span.end}, {f, acc, xs});
            }
            default:
                break;
        }
        fail("expected an expression, found " + describe(peek()));
    }
};

}

ExprPtr parse(const std::string& src) {
    Parser p(lex(src));
    return p.program();
}