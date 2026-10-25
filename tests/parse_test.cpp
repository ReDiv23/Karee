
#include <iostream>
#include <string>

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

static std::string dump(const ExprPtr& e) {
    if (!e) return "<null>";
    return pretty(e).text;
}

static void expect_parse(
    const std::string& src,
    const std::string& expected,
    const std::string& label
) {
    try {
        ExprPtr e = parse(src);
        std::string got = dump(e);

        CHECK(
            got == expected,
            label + ": got `" + got + "`, expected `" + expected + "`"
        );
    } catch (const std::exception& e) {
        CHECK(false, label + ": unexpected parse error: " + e.what());
    }
}

static void expect_success(
    const std::string& src,
    const std::string& label
) {
    try {
        (void)parse(src);
        CHECK(true, label);
    } catch (const std::exception& e) {
        CHECK(false, label + ": unexpected parse error: " + e.what());
    }
}

static void expect_error(
    const std::string& src,
    const std::string& label
) {
    try {
        (void)parse(src);
        CHECK(false, label + ": expected a parse error");
    } catch (const std::exception&) {
        CHECK(true, label);
    }
}

int main() {
    // Numbers and booleans
    expect_parse("42", "42", "integer literal");
    expect_parse("true", "true", "true literal");
    expect_parse("false", "false", "false literal");

    // Variables and lambdas
    expect_parse("x", "x", "variable");
    expect_parse("\\x -> x", "\\x -> x", "lambda");
    expect_parse(
        "\\x y -> x + y",
        "\\x -> \\y -> x + y",
        "multi-argument lambda"
    );

    // Function application
    expect_parse("f x", "f x", "function application");
    expect_parse("f x y", "f x y", "left-associative application");
    expect_parse("f (x + y)", "f (x + y)", "parenthesized argument");

    // Let expressions
    expect_parse("let x = 5 in x", "let x = 5 in x", "let expression");
    expect_parse(
        "let rec f = \\x -> x in f",
        "let rec f = \\x -> x in f",
        "recursive let"
    );

    // Conditionals
    expect_parse(
        "if true then 1 else 2",
        "if true then 1 else 2",
        "if expression"
    );

    // Operator precedence
    expect_parse("1 + 2 * 3", "1 + 2 * 3", "multiplication precedence");
    expect_parse("(1 + 2) * 3", "(1 + 2) * 3", "parentheses");
    expect_parse("1 - 2 - 3", "1 - 2 - 3", "left-associative subtraction");
    expect_parse(
        "1 < 2 && 3 < 4",
        "1 < 2 && 3 < 4",
        "comparison and boolean operators"
    );

    // Unary minus
    expect_parse("-3", "-3", "negative integer");
    expect_parse("-(1 + 2)", "-(1 + 2)", "negated expression");

    // Lists
    expect_parse("[]", "[]", "empty list");
    expect_parse("[1]", "[1]", "single-element list");
    expect_parse("[1, 2, 3]", "[1, 2, 3]", "list literal");
    expect_parse(
        "[1 + 2, 3 * 4]",
        "[1 + 2, 3 * 4]",
        "list with expressions"
    );

    // Cons and concatenation
    expect_parse("1 : [2, 3]", "1 : [2, 3]", "list cons");
    expect_parse(
        "[1, 2] ++ [3, 4]",
        "[1, 2] ++ [3, 4]",
        "list concatenation"
    );

    // Ranges
    expect_parse("[1..5]", "[1..5]", "integer range");
    expect_parse("[3..3]", "[3..3]", "single-element range");
    expect_parse("[5..1]", "[5..1]", "descending range");

    // Built-in list functions
    expect_parse("head [1, 2]", "head [1, 2]", "head");
    expect_parse("tail [1, 2]", "tail [1, 2]", "tail");
    expect_parse("isEmpty []", "isEmpty []", "isEmpty");
    expect_parse("length [1, 2, 3]", "length [1, 2, 3]", "length");
    expect_parse("not true", "not true", "not");

    // Map and fold
    expect_parse(
        "map (\\x -> x * 2) [1, 2]",
        "map (\\x -> x * 2) [1, 2]",
        "map"
    );
    expect_parse(
        "fold (\\a x -> a + x) 0 [1, 2]",
        "fold (\\a -> \\x -> a + x) 0 [1, 2]",
        "fold"
    );

    // Zip
    expect_parse("zip [1, 2] [3, 4]", "zip [1, 2] [3, 4]", "zip");
    expect_parse("zip [] [3, 4]", "zip [] [3, 4]", "zip with empty list");

    // Function composition
    // The printer desugars composition into a generated lambda, so
    // test parsing success rather than comparing the printed source.
    expect_success(
        "(\\x -> x + 1) . (\\x -> x * 2)",
        "function composition parses"
    );

    expect_success(
        "map ((\\x -> x + 1) . (\\x -> x * 2)) [1, 2]",
        "composition as map argument parses"
    );

    // Filter
    expect_parse(
        "filter (\\x -> x > 0) [1, 2, 3]",
        "filter (\\x -> x > 0) [1, 2, 3]",
        "filter"
    );

    expect_parse(
        "filter (\\x -> x % 2 == 0) [1, 2, 3, 4]",
        "filter (\\x -> x % 2 == 0) [1, 2, 3, 4]",
        "filter with modulo"
    );

    // Nested expressions
    expect_parse(
        "map (\\x -> x * 2) "
        "(filter (\\x -> x > 0) [-1, 0, 1, 2])",
        "map (\\x -> x * 2) "
        "(filter (\\x -> x > 0) [-1, 0, 1, 2])",
        "nested map and filter"
    );

    // Malformed inputs
    expect_error("[1..]", "incomplete range");
    expect_error("let x = 1", "let without in");
    expect_error("if true then 1", "if without else");
    expect_error("\\x ->", "lambda without body");
    expect_error("[1, 2", "unterminated list");

    std::cout << (checks - failures) << "/" << checks
              << " checks passed\n";

    return failures ? 1 : 0;
}
