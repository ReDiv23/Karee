#pragma once
#include <string>

inline std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '"':
                o += "\\\"";
                break;
            case '\\':
                o += "\\\\";
                break;
            case '\n':
                o += "\\n";
                break;
            case '\t':
                o += "\\t";
                break;
            case '\r':
                o += "\\r";
                break;
            default:
                o += c;
        }
    }
    return o;
}