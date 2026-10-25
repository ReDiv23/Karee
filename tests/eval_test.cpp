#include <iostream>
#include "eval.h"
#include "parser.h"
#include "printer.h"

static int checks = 0, failures = 0;
#define CHECK(cond, msg) do { checks++; if (!(cond)) { failures++; \
    std::cerr << "FAIL: " << msg << "\n"; } } while (0)

// Runs a program to the end. Returns the final value as text; counts the steps.
static std::string run(const std::string& src, int& steps) {
    ExprPtr cur = parse(src);
    steps = 0;
    while (auto s = step(cur)) { cur = s->next; if (++steps > 10000) return "LOOP"; }
    return pretty(cur).text;
}

int main() {
    struct Ok { const char* src; const char* want; int steps; };
    const Ok oks[] = {
        {"1 + 2 * 3",                         "7",        2},
        {"let x = 2 + 3 in x * x",            "25",       3},
        {"(\\x -> x + 1) 4",                  "5",        2},
        {"(\\x y -> x - y) 10 3",             "7",        3},
        {"if 1 < 2 then 10 else 20",          "10",       2},
        {"[1 + 1, 2 * 3]",                    "[2, 6]",   2},
        {"true && false",                     "false",    1},
        {"false && (1 / 0 == 1)",             "false",    1},   // short-circuit: no division
        {"let x = 1 in let x = 2 in x",       "2",        2},   // shadowing
        {"let x = 5 in (\\x -> x) 7",         "7",        2},   // shadowing by a lambda
        {"(\\f -> f (f 2)) (\\n -> n * 3)",   "18",       5},
        {"let x = 1 in (\\y -> x + y) 2",     "3",        3},
        {"(\\x -> \\y -> x) 1 2",             "1",        2},
        // map
        {"map (\\x -> x * 2) [1, 2, 3]",     "[2, 4, 6]", 7},   // 1 expand + 3 x (beta, arith)
        {"map (\\x -> x + 1) []",            "[]",       1},
        {"map (\\f -> f 3) [\\x -> x + 1, \\x -> x * 2]", "[4, 6]", -1},
        {"let double = \\x -> x * 2 in map double [1, 2]", "[2, 4]", -1},
        // fold
        {"fold (\\a x -> a + x) 0 [1, 2, 3, 4]", "10",   17},   // 4 per element + 1 to finish
        {"fold (\\a x -> a + x) 0 []",       "0",        1},
        {"fold (\\a x -> a + x * x) 0 (map (\\x -> x + 1) [1, 2, 3])", "29", -1},
        {"map (\\xs -> fold (\\a x -> a + x) 0 xs) [[1, 2], [3, 4]]", "[3, 7]", -1},
        // let rec
        {"let rec fact = \\n -> if n == 0 then 1 else n * fact (n - 1) in fact 4", "24", -1},
        {"let rec fib = \\n -> if n < 2 then n else fib (n - 1) + fib (n - 2) in fib 10", "55", -1},
        {"let rec sum = \\xs -> fold (\\a x -> a + x) 0 xs in sum [1, 2, 3]", "6", -1},
        {"let rec f = \\f -> f in f 3",        "3",        -1},   // parameter shadows the name
        {"let rec f = \\n -> n in 5",         "5",        1},    // never used: one step, no unrolling
        // unary minus
        {"let x = 3 in -x",                   "-3",       2},    // bind x, then negate
        {"map (\\x -> -x) [1, 2]",            "[-1, -2]", 5},    // 1 expand + 2 x (beta, negate)
        {"-(1 + 2)",                          "-3",       2},
        {"- -3",                              "3",        1},    // -3 is a literal, so only one negation
        {"1 - -2",                            "3",        1},
        {"-(2 ^ 2)",                          "-4",       2},
        {"let x = 2 in -x ^ 2",               "4",        3},    // unary minus binds tighter than ^: (-x) ^ 2
        {"let f = \\x -> x in -f 3",          "-3",       3},    // -f 3 is -(f 3)
        {"[1, -2, 3]",                        "[1, -2, 3]", 0},  // negative literals are values already
        {"-3 + 4",                            "1",        1},
    };
    for (const Ok& t : oks) {
        int n;
        std::string got = run(t.src, n);
        CHECK(got == t.want, std::string(t.src) + "  got: " + got + "  want: " + t.want);
        if (t.steps >= 0)
            CHECK(n == t.steps,  std::string(t.src) + "  steps: " + std::to_string(n) +
                                 "  want: " + std::to_string(t.steps));
    }

    // runtime errors: message and the path of the offending part
    struct Bad { const char* src; const char* msg; Path where; };
    const Bad bads[] = {
        {"1 / 0",                  "division by zero",                     {1}},
        {"y + 1",                  "unbound variable 'y'",                 {0}},
        {"1 2",                    "cannot call a number as a function",   {0}},
        {"if 3 then 1 else 2",     "the condition of 'if' must be true or false", {0}},
        {"true + 1",               "'+' needs a number, got a boolean",    {0}},
        {"1 + 2 + true",           "'+' needs a number, got a boolean",    {1}},
        {"map 1 [1]",              "map needs a function as its first argument", {0}},
        {"map (\\x -> x) 5",       "map needs a list as its second argument",    {1}},
        {"fold (\\a x -> a) 0 5",  "fold needs a list as its third argument",    {2}},
        {"let rec x = 5 in x",     "'let rec' needs a function (\\x -> ...) on the right-hand side", {0}},
        {"let rec f = \\n -> n in f 1 + undefined", "unbound variable 'undefined'", {1}},
        {"-true",                  "'-' needs a number, got a boolean",    {0}},
        {"-[1]",                   "'-' needs a number, got a list",       {0}},
        {"let x = 0 - 9223372036854775807 - 1 in -x", "integer overflow",  {}},   // -LLONG_MIN
    };
    for (const Bad& b : bads) {
        try {
            int n; run(b.src, n);
            CHECK(false, std::string("expected error for ") + b.src);
        } catch (const RuntimeError& e) {
            CHECK(std::string(e.what()) == b.msg, std::string(b.src) + "  msg: " + e.what());
            CHECK(e.where == b.where, std::string(b.src) + "  wrong error path");
        }
    }

    // evaluation order: leftmost redex first (call-by-value, left to right)
    {
        ExprPtr e = parse("(1 + 2) * (3 + 4)");
        auto s1 = step(e);
        CHECK(s1 && s1->redex == Path({0}), "first redex is the left sum");
        auto s2 = step(s1->next);
        CHECK(s2 && s2->redex == Path({1}), "second redex is the right sum");
        auto s3 = step(s2->next);
        CHECK(s3 && s3->redex == Path({}), "third redex is the whole product");
    }

    // sharing: stepping inside one child must reuse the other child untouched
    {
        ExprPtr e = parse("(1 + 2) * (3 + 4)");
        auto s1 = step(e);
        CHECK(s1->next->kids[1] == e->kids[1], "unchanged subtree is shared, not copied");
    }

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures ? 1 : 0;
}