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

    std::string fresh_comp_name(int pos) const {
        int suffix = 0;

        while (true) {
            std::string candidate = "__karee_comp_" + std::to_string(pos) + "_" + std::to_string(suffix);

            bool used = false;

            for (const Token& token : t_) {
                if (token.type == Tok::Ident && token.text == candidate) {
                    used = true;
                    break;
                }
            }

            if (!used) return candidate;

            ++suffix;
        }
    }

    static bool starts_atom(Tok k) {
        return k == Tok::Num || k == Tok::Ident || k == Tok::True || k == Tok::False || k == Tok::LParen || k == Tok::LBrack || k == Tok::Map || k == Tok::Fold || k == Tok::Zip || k == Tok::Filter;
    }

    ExprPtr expr() {
        if (at(Tok::Let)) {
            Token kw = next();
            bool rec = false;
            if (at(Tok::Rec)) {
                next();
                rec = true;
            }
            Token id = expect(Tok::Ident, "a name");
            expect(Tok::Eq, "'='");
            ExprPtr v = expr();
            expect(Tok::In, "'in'");
            ExprPtr body = expr();
            return mk_node(rec ? Kind::LetRec : Kind::Let, {kw.span.start, body->span.end}, {v, body}, id.text);
        }

        if (at(Tok::Backslash)) {
            Token kw = next();
            std::vector<Token> params;
            params.push_back(expect(Tok::Ident, "a parameter name"));
            while (at(Tok::Ident)) params.push_back(next());
            expect(Tok::Arrow, "'->'");
            ExprPtr e = expr();
            int end = e->span.end;
            for (int k = params.size() - 1; k >= 0; k--) {
                e = mk_node(Kind::Lam, {k == 0 ? kw.span.start : params[k].span.start, end}, {e}, params[k].text);
            }
            return e;
        }

        if (at(Tok::If)) {
            Token kw = next();
            ExprPtr c = expr();
            expect(Tok::Then, "'then'");
            ExprPtr a = expr();
            expect(Tok::Else, "'else'");
            ExprPtr b = expr();
            return mk_node(Kind::If, {kw.span.start, b->span.end}, {c, a, b});
        }

        return binop(1);
    }

    ExprPtr binop(int min_prec) {
        ExprPtr lhs = app();

        while (at(Tok::Op) &&
            binop_prec(peek().text) >= min_prec) {
            Token op = next();
            int p = binop_prec(op.text);

            ExprPtr rhs = binop(is_right_assoc(op.text) ? p : p + 1);

            if (op.text == ".") {
                std::string param = fresh_comp_name(op.span.start);

                Span whole{lhs->span.start, rhs->span.end};

                ExprPtr x = mk_node(Kind::Var, op.span, {}, param);

                ExprPtr gx = mk_node(Kind::App, {rhs->span.start, rhs->span.end}, {rhs, x});

                ExprPtr fx = mk_node(Kind::App, whole, {lhs, gx});

                lhs = mk_node(Kind::Lam, whole, {fx}, param);
            }
            else {
                lhs = mk_node(Kind::BinOp, {lhs->span.start, rhs->span.end}, {lhs, rhs},op.text);
            }
        }

        return lhs;
    }

    ExprPtr app() {
        if (at(Tok::Let) || at(Tok::Backslash) || at(Tok::If)) return expr();
        if (at(Tok::Op) && peek().text == "-" && t_[i_ + 1].type != Tok::Num) {
            Token minus = next();
            ExprPtr operand = app();
            return mk_node(Kind::Neg, {minus.span.start, operand->span.end}, {operand});
        }

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

                if (at(Tok::RBrack)) {
                    Token close = next();
                    return mk_node(Kind::List, {open.span.start, close.span.end}, {});
                }

                ExprPtr first = expr();

                if (at(Tok::DotDot)) {
                    next();

                    ExprPtr last = expr();
                    Token close = expect(Tok::RBrack, "']'");

                    return mk_node(Kind::Range, {open.span.start, close.span.end}, {first, last});
                }

                std::vector<ExprPtr> items;
                items.push_back(first);

                while (at(Tok::Comma)) {
                    next();
                    items.push_back(expr());
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
            case Tok::Zip: {
                Token kw = next();
                ExprPtr xs = atom();
                ExprPtr ys = atom();

                return mk_node(Kind::Zip, {kw.span.start, ys->span.end}, {xs, ys});
            }
            case Tok::Filter: {
                Token kw = next();

                ExprPtr predicate = atom();
                ExprPtr xs = atom();

                return mk_node(Kind::Filter, {kw.span.start, xs->span.end}, {predicate, xs});
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

std::pair<int, int> line_col(const std::string& src, int pos) {
    int line = 1, col = 1;
    for (int i = 0; i < pos && i < (int)src.size(); i++) {
        if (src[i] == '\n') {
            line++;
            col = 1;
        }
        else col++;
    }
    return {line, col};
}