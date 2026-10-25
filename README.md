# Karee 『カレー』

Watch a tiny functional program run, one reduction at a time.

Karee is an interpreter for a small Haskell-like language (numbers, booleans, lists, `let`, functions, `map`, `fold`). It rewrites the expression in place, one reduction per step, and shows you exactly which part is being reduced, as highlighted code or as a syntax tree. Lists appear as rows of boxes, and the box being worked on lights up.

<!-- Add a screenshot or GIF here: docs/demo.gif -->

## Features

- **Step-by-step evaluation.** Every click applies exactly one reduction, with a rule name (`beta`, `let`, `arith`, `map-expand`, `fold-step`, ...) and a one-line explanation.
- **Two views.** *Code* highlights the redex in the source text. *Tree* draws the AST with the node being reduced highlighted and the path down to it marked. The tree supports scroll-to-zoom, drag-to-pan and a Fit button.
- **Boxes for `map` and `fold`.** `map` lights every box it is about to apply the function to. `fold` shows an accumulator that eats one box at a time.
- **Full timeline.** Play, pause, step forward and back, jump to start or end, scrub, change speed. Keyboard: arrows, Home, End, Space, Ctrl/Cmd+Enter to run.
- **Example gallery** with factorial, Fibonacci, currying, map feeding fold, a runtime error and an infinite loop.
- **Your own programs.** Editing an example makes a copy under *My programs*. Create new ones (＋), save copies (⧉), rename (✎) and delete (✕).
- **Shareable links.** The program is stored in the URL hash.
- **Friendly errors.** Parse errors show the line, column and a caret. Runtime errors stop at the offending subexpression, highlighted in red.
- **Runs entirely in the browser.** The interpreter is C++ compiled to WebAssembly. There is no server.

## The language

```haskell
-- comments start with two dashes
let x = 2 + 3 in x * x

map (\x -> x * 2) [1, 2, 3]

fold (\acc x -> acc + x) 0 [1, 2, 3, 4]

let rec fact = \n -> if n == 0 then 1 else n * fact (n - 1) in fact 4
```

| Construct | Syntax |
|---|---|
| Numbers, booleans | `42`, `-3`, `true`, `false` (64-bit integers; overflow is an error) |
| Lists | `[1, 2, 3]`, `[]` |
| Functions | `\x -> x + 1`, `\x y -> x + y` (curried) |
| Application | `f x y` |
| Let | `let x = 1 in x + 1`, `let rec f = \n -> ... in f 3` |
| Conditionals | `if c then a else b` |
| Operators | `+ - * / % ^`, `< > <= >= == !=`, `&& \|\|` (`&&` and `\|\|` short-circuit) |
| Map | `map f xs` |
| Fold | `fold f acc xs` (a left fold) |

`^` is power (right-associative, whole-number exponents of 0 or more). `%` is the remainder with the sign of the divisor, like Haskell's `mod`. `/` is integer division and rounds toward zero. From tightest to loosest: `^`, `* / %`, `+ -`, comparisons, `&&`, `||`.

`map`, `fold` and the built-ins are keywords that take their arguments directly, so wrap compound arguments in parentheses: `fold f 0 (map g xs)`.

Evaluation is strict (call-by-value), left to right, using substitution. There are no strings and no pattern matching.

## Run it locally

You need [Emscripten](https://emscripten.org/docs/getting_started/downloads.html).

```sh
source ~/emsdk/emsdk_env.sh      # activate Emscripten in this terminal
./build_wasm.sh                  # produces web/karee.js and web/karee.wasm
python3 -m http.server -d web 8000
```

Open <http://localhost:8000>. Use `http://`, not `file://`, because the browser will not load the `.wasm` file from disk.

If the page looks stale after a change, hard-refresh (Ctrl/Cmd+Shift+R).

### Command-line trace (no Emscripten needed)

```sh
g++ -std=c++17 -Isrc src/main.cpp src/api.cpp src/lexer.cpp src/parser.cpp \
    src/printer.cpp src/eval.cpp src/boxes.cpp src/tree.cpp -o karee

echo 'let x = 2 + 3 in x * x' | ./karee -t     # human-readable trace
echo 'map (\x -> x * 2) [1, 2]' | ./karee       # the JSON the web app consumes
```

## How it works

```
source text ──lex──▶ tokens ──parse──▶ AST ──step()──▶ AST ──step()──▶ ... ──▶ value
                                         │
                       each step also records: redex path, rule, caption
                                         ▼
                      pretty-printer ─▶ text + redex span      (Code view)
                      boxes.cpp      ─▶ list / map / fold boxes
                      tree.cpp       ─▶ AST with the redex marked (Tree view)
                                         ▼
                              one JSON document, all steps
                                         ▼
                           browser: Player + renderers
```

- **`step(expr)`** (`src/eval.cpp`) finds the leftmost-innermost redex, rewrites it, and returns the new expression plus the *path* to the redex (a list of child indices). Everything visual is derived from that path.
- **Immutable ASTs.** Nodes are shared and never changed, so keeping every step costs little and stepping backward is free.
- **One run, many steps.** `run_program` evaluates to completion (or to the 5000-step limit) and returns every step as JSON. The browser's `Player` just walks that array.

### Layout

```
src/
  ast.h        expression type, spans, paths
  lexer.*      tokens and spans
  parser.*     recursive-descent parser
  eval.*       is_value, substitution, small-step reducer
  printer.*    expression -> text, and the redex's text span
  boxes.*      JSON for the list / map / fold boxes
  tree.*       JSON for the syntax-tree view
  api.*        run_program: source -> JSON of all steps
  wasm_bind.cpp  exports run() to JavaScript
  main.cpp     command-line trace
web/
  index.html, style.css
  app.js       UI, program library, rendering
  player.js    timeline state
  tree.js      SVG tree with zoom and pan
  examples.js  the example gallery
build_wasm.sh
```

## Limits

- Runs stop after 5000 steps, which catches infinite loops, and the UI says so.
- Very large intermediate expressions make the tree wide. Use zoom and Fit.
- Programs you write live in memory and are lost on reload. Share a link to keep one.