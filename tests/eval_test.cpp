
#include <iostream>
#include <string>

#include "eval.h"
#include "parser.h"
#include "printer.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond, msg) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        std::cerr << "FAIL: " << msg << "\n"; \
    } \
} while (0)

// Evaluate a program to completion and count the steps.
static std::string run(const std::string& src, int& steps) {
    ExprPtr cur = parse(src);
    steps = 0;

    while (auto s = step(cur)) {
        cur = s->next;
        if (++steps > 10000)
            return "LOOP";
    }

    return pretty(cur).text;
}

int main() {
    struct Ok {
        const char* src;
        const char* want;
        int steps; // -1 means don't check exact step count
    };

    const Ok oks[] = {
        // Basic arithmetic
        {"1 + 2 * 3", "7", 2},
        {"let x = 2 + 3 in x * x", "25", 3},
        {"(\\x -> x + 1) 4", "5", 2},
        {"(\\x y -> x - y) 10 3", "7", 3},
        {"if 1 < 2 then 10 else 20", "10", 2},

        // Lists and booleans
        {"[1 + 1, 2 * 3]", "[2, 6]", 2},
        {"true && false", "false", 1},
        {"false && (1 / 0 == 1)", "false", 1},
        {"[1, -2, 3]", "[1, -2, 3]", 0},

        // Scope and functions
        {"let x = 1 in let x = 2 in x", "2", 2},
        {"let x = 5 in (\\x -> x) 7", "7", 2},
        {"(\\f -> f (f 2)) (\\n -> n * 3)", "18", 5},
        {"let x = 1 in (\\y -> x + y) 2", "3", 3},
        {"(\\x -> \\y -> x) 1 2", "1", 2},

        // Map
        {"map (\\x -> x * 2) [1, 2, 3]", "[2, 4, 6]", 7},
        {"map (\\x -> x + 1) []", "[]", 1},
        {"map (\\f -> f 3) [\\x -> x + 1, \\x -> x * 2]", "[4, 6]", -1},
        {"let double = \\x -> x * 2 in map double [1, 2]", "[2, 4]", -1},

        // Fold
        {"fold (\\a x -> a + x) 0 [1, 2, 3, 4]", "10", 17},
        {"fold (\\a x -> a + x) 0 []", "0", 1},
        {"fold (\\a x -> a + x * x) 0 (map (\\x -> x + 1) [1, 2, 3])", "29", -1},
        {"map (\\xs -> fold (\\a x -> a + x) 0 xs) [[1, 2], [3, 4]]", "[3, 7]", -1},

        // Recursion
        {"let rec fact = \\n -> if n == 0 then 1 else n * fact (n - 1) in fact 4", "24", -1},
        {"let rec fib = \\n -> if n < 2 then n else fib (n - 1) + fib (n - 2) in fib 10", "55", -1},
        {"let rec sum = \\xs -> fold (\\a x -> a + x) 0 xs in sum [1, 2, 3]", "6", -1},
        {"let rec f = \\f -> f in f 3", "3", -1},
        {"let rec f = \\n -> n in 5", "5", 1},

        // Unary minus
        {"let x = 3 in -x", "-3", 2},
        {"map (\\x -> -x) [1, 2]", "[-1, -2]", 5},
        {"-(1 + 2)", "-3", 2},
        {"- -3", "3", 1},
        {"1 - -2", "3", 1},
        {"-(2 ^ 2)", "-4", 2},
        {"let x = 2 in -x ^ 2", "4", 3},
        {"let f = \\x -> x in -f 3", "-3", 3},
        {"-3 + 4", "1", 1},

        // List operators and built-ins
        {"1 : [2, 3]", "[1, 2, 3]", -1},
        {"[1, 2] ++ [3, 4]", "[1, 2, 3, 4]", -1},
        {"head [10, 20, 30]", "10", -1},
        {"tail [10, 20, 30]", "[20, 30]", -1},
        {"isEmpty []", "true", -1},
        {"isEmpty [1]", "false", -1},
        {"length []", "0", -1},
        {"length [2, 4, 6, 8]", "4", -1},
        {"not true", "false", -1},
        {"not false", "true", -1},
        {"let rec len = \\xs -> if isEmpty xs then 0 else 1 + len (tail xs) in len [5, 10, 15, 20]", "4", -1},

        // Inclusive ranges
        {"[1..5]", "[1, 2, 3, 4, 5]", -1},
        {"[3..3]", "[3]", -1},
        {"[5..1]", "[]", -1},
        {"map (\\x -> x * 2) [1..5]", "[2, 4, 6, 8, 10]", -1},

        // Zip
        {"zip [1, 2, 3] [10, 20, 30]", "[[1, 10], [2, 20], [3, 30]]", -1},
        {"zip [1, 2, 3] [10, 20]", "[[1, 10], [2, 20]]", -1},
        {"zip [] [10, 20]", "[]", -1},
        {"zip [1, 2] []", "[]", -1},

        // Function composition
        {"map ((\\x -> x + 1) . (\\x -> x * 2)) [1, 2, 3]", "[3, 5, 7]", -1},
        {"map ((\\x -> x + 1) . (\\x -> x * 2) . (\\x -> x * x)) [1, 2, 3]", "[3, 9, 19]", -1},

        // Filter
        {"filter (\\x -> x % 2 == 0) [1, 2, 3, 4, 5, 6]", "[2, 4, 6]", -1},
        {"filter (\\x -> x > 0) [-2, -1, 0, 1, 2, 3]", "[1, 2, 3]", -1},
        {"filter (\\x -> x > 0) []", "[]", -1},
        {"filter (\\x -> x == 3) [1, 2, 3, 4, 5]", "[3]", -1},
        {"filter (\\x -> x < 0) [1, 2, 3]", "[]", -1},
        {"map (\\x -> x * 2) (filter (\\x -> x % 2 == 0) [1, 2, 3, 4, 5, 6])", "[4, 8, 12]", -1},
    };

    for (const Ok& t : oks) {
        try {
            int n = 0;
            std::string got = run(t.src, n);

            CHECK(
                got == t.want,
                std::string(t.src) + " got: " + got +
                " want: " + t.want
            );

            if (t.steps >= 0) {
                CHECK(
                    n == t.steps,
                    std::string(t.src) + " steps: " +
                    std::to_string(n) + " want: " +
                    std::to_string(t.steps)
                );
            }
        } catch (const std::exception& e) {
            CHECK(false, std::string(t.src) + " threw: " + e.what());
        }
    }

    // Runtime errors: check both the error message and its source path.
    struct Bad {
        const char* src;
        const char* msg;
        Path where;
    };

    const Bad bads[] = {
        {"1 / 0", "division by zero", {1}},
        {"y + 1", "unbound variable 'y'", {0}},
        {"1 2", "cannot call a number as a function", {0}},
        {"if 3 then 1 else 2", "the condition of 'if' must be true or false", {0}},
        {"true + 1", "'+' needs a number, got a boolean", {0}},
        {"1 + 2 + true", "'+' needs a number, got a boolean", {1}},
        {"map 1 [1]", "map needs a function as its first argument", {0}},
        {"map (\\x -> x) 5", "map needs a list as its second argument", {1}},
        {"fold (\\a x -> a) 0 5", "fold needs a list as its third argument", {2}},
        {"let rec x = 5 in x", "'let rec' needs a function (\\x -> ...) on the right-hand side", {0}},
        {"let rec f = \\n -> n in f 1 + undefined", "unbound variable 'undefined'", {1}},
        {"-true", "'-' needs a number, got a boolean", {0}},
        {"-[1]", "'-' needs a number, got a list", {0}},
        {"let x = 0 - 9223372036854775807 - 1 in -x", "integer overflow", {}},
    };

    for (const Bad& b : bads) {
        try {
            int n = 0;
            run(b.src, n);
            CHECK(false, std::string("expected error for ") + b.src);
        } catch (const RuntimeError& e) {
            CHECK(std::string(e.what()) == b.msg,
                  std::string(b.src) + " message: " + e.what());
            CHECK(e.where == b.where,
                  std::string(b.src) + " wrong error path");
        } catch (const std::exception& e) {
            CHECK(false, std::string(b.src) + " unexpected exception: " + e.what());
        }
    }

    // Evaluation order: leftmost redex first.
    {
        ExprPtr e = parse("(1 + 2) * (3 + 4)");
        auto s1 = step(e);

        CHECK(s1.has_value(), "first step exists");

        if (s1) {
            CHECK(s1->redex == Path({0}), "first redex is the left sum");

            auto s2 = step(s1->next);
            CHECK(s2.has_value(), "second step exists");

            if (s2) {
                CHECK(s2->redex == Path({1}), "second redex is the right sum");

                auto s3 = step(s2->next);
                CHECK(s3.has_value(), "third step exists");

                if (s3)
                    CHECK(s3->redex == Path({}),
                          "third redex is the whole product");
            }
        }
    }

    // Unchanged subtrees should be shared.
    {
        ExprPtr e = parse("(1 + 2) * (3 + 4)");
        auto s1 = step(e);

        CHECK(s1.has_value(), "first step exists");

        if (s1) {
            CHECK(
                s1->next->kids[1] == e->kids[1],
                "unchanged subtree is shared, not copied"
            );
        }
    }

    // Filter trace should contain keep/drop decisions.
    {
        try {
            ExprPtr cur = parse("filter (\\x -> x % 2 == 0) [1, 2]");
            bool sawKeep = false;
            bool sawDrop = false;
            int guard = 0;

            while (auto s = step(cur)) {
                sawKeep = sawKeep || s->rule == "filter-keep";
                sawDrop = sawDrop || s->rule == "filter-drop";
                cur = s->next;

                if (++guard > 1000)
                    break;
            }

            CHECK(sawKeep, "filter trace contains a filter-keep step");
            CHECK(sawDrop, "filter trace contains a filter-drop step");
            CHECK(pretty(cur).text == "[2]",
                  "filter trace ends with retained elements");
        } catch (const std::exception& e) {
            CHECK(false, std::string("filter trace threw: ") + e.what());
        }
    }

    std::cout << (checks - failures) << "/" << checks
              << " checks passed\n";

    return failures ? 1 : 0;
}
