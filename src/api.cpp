#include "api.h"
#include "json.h"
#include "parser.h"
#include "printer.h"

std::string run_program(const std::string& source) {
    try {
        ExprPtr e = parse(source);
        Printed p = pretty(e);
        return "{\"ok\":true,\"steps\":[{\"text\":\"" + json_escape(p.text) + "\",\"redex\":[0," + std::to_string(p.text.size()) + "],\"rule\":\"parse\",\"caption\":\"Parsed OK\",\"boxes\":null}]}";
    } catch (const ParseError& err) {
        auto [line, col] = line_col(source, err.span.start);
        return "{\"ok\":false,\"steps\":[],\"error\":{\"msg\":\"" + json_escape(err.what()) + "\",\"span\":[" + std::to_string(err.span.start) + "," + std::to_string(err.span.end) + "],\"line\":" + std::to_string(line) + ",\"col\":" + std::to_string(col) + "}}";
    }
}