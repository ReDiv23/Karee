#include <iostream>
#include "boxes.h"
#include "eval.h"
#include "parser.h"

static int checks = 0, failures = 0;
#define CHECK(cond, msg) do { checks++; if (!(cond)) { failures++; \
    std::cerr << "FAIL: " << msg << "\n"; } } while (0)

static bool has(const std::string& s, const std::string& part) { return s.find(part) != std::string::npos; }

// boxes JSON for the n-th step (0-based) of a program
static std::string boxes_at(const std::string& src, int n) {
    ExprPtr cur = parse(src);
    for (int i = 0;; i++) {
        auto s = step(cur);
        if (!s) return boxes_json(cur, nullptr);
        if (i == n) return boxes_json(cur, &s->redex);
        cur = s->next;
    }
}

int main() {
    // no list involved
    CHECK(boxes_at("1 + 2", 0) == "null", "plain arithmetic has no boxes");

    // fold: the head box is absorbed, then the accumulator reduces
    std::string f0 = boxes_at("fold (\\a x -> a + x) 0 [1, 2, 3]", 0);
    CHECK(has(f0, "\"kind\":\"fold\"") && has(f0, "\"acc\":\"0\"") &&
          has(f0, "\"items\":[\"1\",\"2\",\"3\"]") && has(f0, "\"active\":0"), "fold step 0: " + f0);
    std::string f1 = boxes_at("fold (\\a x -> a + x) 0 [1, 2, 3]", 1);
    CHECK(has(f1, "\"items\":[\"2\",\"3\"]") && has(f1, "\"accActive\":true") && has(f1, "\"active\":-1"),
          "fold step 1 (list shrank, acc is working): " + f1);

    // map: first all boxes light up, then one box at a time
    std::string m0 = boxes_at("map (\\x -> x * 2) [1, 2, 3]", 0);
    CHECK(has(m0, "\"kind\":\"map\"") && has(m0, "\"all\":true") && has(m0, "\"items\":[\"1\",\"2\",\"3\"]"),
          "map expand: " + m0);
    std::string m1 = boxes_at("map (\\x -> x * 2) [1, 2, 3]", 1);
    CHECK(has(m1, "\"kind\":\"list\"") && has(m1, "\"active\":0"), "first box active: " + m1);
    std::string m3 = boxes_at("map (\\x -> x * 2) [1, 2, 3]", 3);
    CHECK(has(m3, "\"active\":1"), "second box active: " + m3);

    // finished list: shown as boxes, nothing lit
    std::string done = boxes_at("map (\\x -> x * 2) [1, 2]", 99);
    CHECK(has(done, "\"kind\":\"list\"") && has(done, "\"items\":[\"2\",\"4\"]") && has(done, "\"active\":-1"),
          "final list: " + done);

    // innermost container wins: a fold whose list is being produced by a map
    std::string nested = boxes_at("fold (\\a x -> a + x) 0 (map (\\x -> x + 1) [1, 2])", 0);
    CHECK(has(nested, "\"kind\":\"map\""), "nested: the map is the focus: " + nested);

    std::cout << (checks - failures) << "/" << checks << " checks passed\n";
    return failures ? 1 : 0;
}