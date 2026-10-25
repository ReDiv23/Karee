#include "lexer.h"
#include <cctype>
#include <climits>
#include <unordered_map>

std::vector<Token> lex(const std::string& src) {
    static const std::unordered_map<std::string, Tok> keywords = {
        {"let", Tok::Let},
        {"rec", Tok::Rec},
        {"in", Tok::In},
        {"if", Tok::If},
        {"then", Tok::Then},
        {"else", Tok::Else},
        {"map", Tok::Map},
        {"fold", Tok::Fold},
        {"true", Tok::True},
        {"false", Tok::False}
    };

    std::vector<Token> out;
    const size_t n = src.size();
    size_t i = 0;

    auto emit = [&](Tok t, size_t a, size_t b) {
        Token k;
        k.type = t;
        k.text = src.substr(a, b - a);
        k.span = {(int)a, (int)b};
        out.push_back(k);
    };

    while (i < n) {
        char c = src[i];

        if (std::isspace((unsigned char)c)) {
            i++;
            continue;
        }

        if (c == '-' && i + 1 < n && src[i + 1] == '-') {
            while (i < n && src[i] != '\n') i++;
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            size_t a = i;
            long long v = 0;
            while (i < n && std::isdigit((unsigned char)src[i])) {
                int d = src[i] - '0';
                if (v > (LLONG_MAX - d) / 10) throw ParseError("number too large", {(int)a, (int)i + 1});
                v = v * 10 + d;
                i++;
            }
            emit(Tok::Num, a, i);
            out.back().num = v;
            continue;
        }

        if (std::isalpha((unsigned char)c) || c == '_') {
            size_t a = i;
            while (i < n && (std::isalnum((unsigned char)src[i]) || src[i] == '_' || src[i] == '\'')) i++;
            auto it = keywords.find(src.substr(a, i - a));
            emit(it == keywords.end() ? Tok::Ident : it->second, a, i);
            continue;
        }

        size_t a = i;
        char nx = (i + 1 < n) ? src[i + 1] : '\0';
        switch (c) {
            case '\\':
                i++;
                emit(Tok::Backslash, a, i);
                break;
            case '(':
                i++;
                emit(Tok::LParen, a, i);
                break;
            case ')': 
                i++;
                emit(Tok::RParen, a, i);
                break;
            case '[':
                i++;
                emit(Tok::LBrack, a, i);
                break;
            case ']':
                i++;
                emit(Tok::RBrack, a, i);
                break;
            case ',':
                i++;
                emit(Tok::Comma, a, i);
                break;
            case '-':
                if (nx == '>') {
                    i += 2;
                    emit(Tok::Arrow, a, i);
                }
                else {
                    i++;
                    emit(Tok::Op, a, i);
                }
                break;
            case '=':
                if (nx == '=') {
                    i += 2;
                    emit(Tok::Op, a, i);
                }
                else {
                    i++;
                    emit(Tok::Eq, a, i);
                }
                break;
            case '+':
            case '*':
            case '/':
            case '%':
            case '^':
                i++; emit(Tok::Op, a, i); break;
            case '<':
            case '>':
                if (nx == '=') i += 2; else i++;
                emit(Tok::Op, a, i);
                break;
            case '!':
                if (nx == '=') {
                    i += 2;
                    emit(Tok::Op, a, i);
                    break;
                }
                throw ParseError("unexpected '!' (did you mean '!='?)", {(int)a, (int)a + 1});
            case '&':
                if (nx == '&') {
                    i += 2;
                    emit(Tok::Op, a, i);
                    break;
                }
                throw ParseError("unexpected '&' (did you mean '&&'?)", {(int)a, (int)a + 1});
            case '|':
                if (nx == '|') {
                    i += 2;
                    emit(Tok::Op, a, i);
                    break;
                }
                throw ParseError("unexpected '|' (did you mean '||'?)", {(int)a, (int)a + 1});
            default:
                throw ParseError(std::string("unexpected character '") + c + "'", {(int)a, (int)a + 1});
        }
    }

    Token end;
    end.type = Tok::End;
    end.span = {(int)n, (int)n};
    out.push_back(end);
    return out;
}