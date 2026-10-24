#include <iostream>
#include "parser.h"
#include "printer.h"

static int checks = 0, failures = 0;
#define CHECK(cond, msg) do { checks++; if (!(cond)) { failures++; \
    std::cerr << "FAIL: " << msg << "\n"; } } while (0)

int main() {
    struct Case { const char* src; const char* want; };
    const Case cases[] = {
        {R"~(let x = 2+1 in x+1)~",             R"~(let x = 2 + 1 in x + 1)~"},
        {R"~(map (\x -> x*2) [1,2,3])~",        R"~(map (\x -> x * 2) [1, 2, 3])~"},
        {R"~(fold (\a x -> a + x) 0 [1,2,3])~", R"~(fold (\a -> \x -> a + x) 0 [1, 2, 3])~"},
        {R"~(1 - 2 - 3)~",                      R"~(1 - 2 - 3)~"},
        {R"~(1 - (2 - 3))~",                    R"~(1 - (2 - 3))~"},
        {R"~((1+2)*3)~",                        R"~((1 + 2) * 3)~"},
        {R"~(f (g x))~",                        R"~(f (g x))~"},
        {R"~(-3 + 4)~",                         R"~((-3) + 4)~"},
        {R"~(1 + \x -> x)~",                    R"~(1 + (\x -> x))~"},
        {R"~(let rec fact = \n -> if n == 0 then 1 else n * fact (n-1) in fact 4)~",
         R"~(let rec fact = \n -> if n == 0 then 1 else n * fact (n - 1) in fact 4)~"},
    };

    for (const Case& c : cases) {
        std::string once  = pretty(parse(c.src)).text;
        std::string twice = pretty(parse(once)).text;      // print -> parse -> print
        CHECK(once == c.want, std::string("input: ") + c.src + "\n  got:  " + once +
                              "\n  want: " + c.want);
        CHECK(twice == once,  std::string("round trip changed: ") + c.src);
    }

    // redex ranges: does the printer point at the right characters?
    {
        ExprPtr e = parse("1 + (2 * 3)");
        Path p = {1};                                       // right child of the +
        Printed r = pretty(e, &p);
        CHECK(r.text.substr(r.redex.start, r.redex.end - r.redex.start) == "2 * 3",
              "redex at path [1]");

        Path root = {};                                     // empty path = whole expression
        Printed r2 = pretty(e, &root);
        CHECK(r2.redex.start == 0 && r2.redex.end == (int)r2.text.size(), "redex at root");

        ExprPtr m = parse("map f [1, 2, 3]");
        Path third = {1, 2};                                // list is child 1, element is child 2
        Printed r3 = pretty(m, &third);
        CHECK(r3.text.substr(r3.redex.start, r3.redex.end - r3.redex.start) == "3",
              "redex at list element");
    }

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures ? 1 : 0;
}