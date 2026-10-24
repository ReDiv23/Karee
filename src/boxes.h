#pragma once
#include <string>
#include "ast.h"

//   {"kind":"list","items":[..],"active":i}
//   {"kind":"map","fn":"..","items":[..],"all":bool}
//   {"kind":"fold","fn":"..","acc":"..","items":[..],"active":i,"accActive":bool}
std::string boxes_json(const ExprPtr& root, const Path* redex);