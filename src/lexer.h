#pragma once

#include <string>
#include <vector>
#include "ast.h"

enum class Tok { Num, Ident, Let, Rec, In, If, Then, Else, Map, Fold, True, False, Backslash, Arrow, LParen, RParen, LBrack, RBrack, Comma, Eq, Op, End };

struct Token {
    Tok type = Tok::End;
    std::string text;
    long long num = 0;
    Span span;
};

std::vector<Token> lex(const std::string& src);