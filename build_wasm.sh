#!/bin/sh
# Needs Emscripten active in this terminal:  source ~/emsdk/emsdk_env.sh
set -e
em++ -std=c++17 -O2 -fexceptions -Isrc \
  src/wasm_bind.cpp src/api.cpp src/lexer.cpp src/parser.cpp src/printer.cpp src/eval.cpp src/boxes.cpp \
  -o web/stepper.js \
  -sMODULARIZE=1 -sEXPORT_NAME=createStepper \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576
echo "built web/stepper.js + web/stepper.wasm"