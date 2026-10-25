#include "boxes.h"
#include "json.h"
#include "printer.h"

namespace {

std::string quote(const ExprPtr& e) { return "\"" + json_escape(pretty(e).text) + "\""; }

std::string items_json(const ExprPtr& list) {
    std::string s = "[";
    if (list->kind == Kind::List) {
        for (size_t i = 0; i < list->kids.size(); i++) {
            if (i) s += ",";
            s += quote(list->kids[i]);
        }
    }
    return s + "]";
}

std::string decisions_json(const ExprPtr& decisions) {
    std::string s = "[";

    if (decisions->kind == Kind::List) {
        for (size_t i = 0; i < decisions->kids.size(); ++i) {
            if (i) s += ",";

            const ExprPtr& d = decisions->kids[i];
            s += d->kind == Kind::Bool && d->boolean ? "true" : "false";
        }
    }

    return s + "]";
}

}

std::string boxes_json(const ExprPtr& root, const Path* redex) {
    static const Path none;
    const Path& path = redex ? *redex : none;

    ExprPtr focus;
    size_t depth = 0;
    ExprPtr cur = root;
    for (size_t d = 0;; d++) {
        if (cur->kind == Kind::List || cur->kind == Kind::Map || cur->kind == Kind::Fold || cur->kind == Kind::Filter) {
            focus = cur;
            depth = d;
        }
        if (d >= path.size()) break;
        int idx = path[d];
        if (idx < 0 || idx >= (int)cur->kids.size()) break;
        cur = cur->kids[idx];
    }
    if (!focus) return "null";

    bool inside = path.size() > depth;
    int next = inside ? path[depth] : -1;

    switch (focus->kind) {
        case Kind::List:
            return "{\"kind\":\"list\",\"items\":" + items_json(focus) + ",\"active\":" + std::to_string(next) + "}";
        case Kind::Map:
            return "{\"kind\":\"map\",\"fn\":" + quote(focus->kids[0]) + ",\"items\":" + items_json(focus->kids[1]) + ",\"all\":" + ((redex && !inside) ? "true" : "false") + "}";
        case Kind::Fold: {
            const ExprPtr& xs = focus->kids[2];
            bool absorbing = redex && !inside && xs->kind == Kind::List && !xs->kids.empty();
            return "{\"kind\":\"fold\",\"fn\":" + quote(focus->kids[0]) + ",\"acc\":" + quote(focus->kids[1]) + ",\"items\":" + items_json(xs) + ",\"active\":" + (absorbing ? "0" : "-1") + ",\"accActive\":" + ((inside && next == 1) ? "true" : "false") + "}";
        }
        case Kind::Filter: {
            bool state = focus->name == "$state";

            const ExprPtr& predicate = focus->kids[0];
            const ExprPtr& original = state ? focus->kids[3] : focus->kids[1];
            const ExprPtr& decisions = state ? focus->kids[4] : mk_node(Kind::List, {}, {});

            size_t processed = decisions->kids.size();

            int active = -1;
            if (original->kind == Kind::List && processed < original->kids.size()) {
                active = static_cast<int>(processed);
            }

            return "{\"kind\":\"filter\",\"fn\":" + quote(predicate) + ",\"items\":" + items_json(original) + ",\"decisions\":" + decisions_json(decisions) + ",\"active\":" + std::to_string(active) + "}";
        }
        default:
            break;
    }
    return "null";
}