#include <vector>

#include "api.h"
#include "eval.h"
#include "json.h"
#include "parser.h"
#include "printer.h"
#include "boxes.h"
#include "tree.h"

namespace {

const int MAX_STEPS = 5000;

std::string step_json(const std::string& text, Span r, const std::string& rule, const std::string& caption, const std::string& boxes = "null", const std::string& tree = "null") {
return "{\"text\":\"" + json_escape(text) + "\",\"redex\":[" + std::to_string(r.start) + "," + std::to_string(r.end) + "],\"rule\":\"" + rule + "\",\"caption\":\"" + json_escape(caption) + "\",\"boxes\":" + boxes + ",\"tree\":" + tree + "}";
}

std::string join(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); i++) { if (i) s += ","; s += v[i]; }
    return s;
}

}

std::string run_program(const std::string& source) {
    ExprPtr cur;
    try {
        cur = parse(source);
    }
    catch (const ParseError& err) {
        auto [line, col] = line_col(source, err.span.start);
        return "{\"ok\":false,\"steps\":[],\"error\":{\"kind\":\"parse\",\"msg\":\"" + json_escape(err.what()) + "\",\"span\":[" + std::to_string(err.span.start) + "," + std::to_string(err.span.end) + "],\"line\":" + std::to_string(line) + ",\"col\":" + std::to_string(col) + "}}";
    }

    std::vector<std::string> steps;
    bool truncated = false;
    std::string error;

    try {
        for (int n = 0;; n++) {
            std::optional<Step> s = step(cur);
            if (!s) {
                steps.push_back(step_json(pretty(cur).text, {-1, -1}, "done", "Final value", boxes_json(cur, nullptr), tree_json(cur, nullptr)));
                break;
            }
            if (n >= MAX_STEPS) {
                truncated = true;
                steps.push_back(step_json(pretty(cur).text, {-1, -1}, "stopped", "Stopped after " + std::to_string(MAX_STEPS) + " steps (possible infinite loop)", "null", tree_json(cur, nullptr)));
                break;
            }
            Printed p = pretty(cur, &s->redex);
            steps.push_back(step_json(p.text, p.redex, s->rule, s->caption, boxes_json(cur, &s->redex), tree_json(cur, &s->redex)));
            cur = s->next;
        }
    }
    catch (const RuntimeError& err) {
        Printed p = pretty(cur, &err.where);
        steps.push_back(step_json(p.text, p.redex, "error", err.what(), boxes_json(cur, &err.where), tree_json(cur, &err.where)));
        error = "{\"kind\":\"runtime\",\"msg\":\"" + json_escape(err.what()) + "\"}";
    }

    return std::string("{\"ok\":") + (error.empty() ? "true" : "false") + ",\"truncated\":" + (truncated ? "true" : "false") + ",\"steps\":[" + join(steps) + "]" + (error.empty() ? "" : ",\"error\":" + error) + "}";
}