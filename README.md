# Karee 『カレー』

Watch a tiny functional program run, one reduction at a time.

Karee is an interpreter for a small Haskell-like language: numbers, booleans, lists and ranges, `let` and `let rec`, lambdas with currying and composition, and `map`, `fold`, `filter` and `zip`. It rewrites the expression in place, one reduction per step, and shows you exactly which part is being reduced, as highlighted code or as a syntax tree. Lists appear as rows of boxes, and the box being worked on lights up.

## Run it

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

### Tests

```sh
for t in parse printer eval boxes; do
  g++ -std=c++17 -Isrc tests/${t}_test.cpp src/lexer.cpp src/parser.cpp \
      src/printer.cpp src/eval.cpp src/boxes.cpp -o /tmp/${t}_test && /tmp/${t}_test
done
```

Each test program prints how many checks passed. `tests/lex_test.cpp` and `tests/scratch.cpp` are small scratch programs, not test suites.

## Features

- **Step-by-step evaluation.** Every click applies exactly one reduction, with a rule name (`beta`, `let`, `arith`, `map-expand`, `fold-step`, `filter-keep`, ...) and a one-line explanation.
- **Two views.** *Code* highlights the redex in the source text. *Tree* draws the AST with the node being reduced highlighted and the path down to it marked.
  - The page widens in Tree view and the tree area is tall, so large trees have room.
  - Scroll to zoom and drag to pan. **Fit** shows the whole tree. **Follow** zooms to the node being reduced and keeps it centred as you step, which is the easiest way to read a big tree.
- **Boxes for lists.** A list is a row of boxes and the box being worked on lights up.
  - `map` lights every box it is about to apply the function to.
  - `fold` shows an accumulator that eats one box at a time.
  - `filter` lights the box being tested and marks the boxes it has dropped.
  - Ranges expand into boxes, and `zip` produces a row of pairs.
- **Full timeline.** Play, pause, step forward and back, jump to start or end, scrub, change speed. Keyboard: arrows, Home, End, Space, Ctrl/Cmd+Enter to run.
- **Example gallery.** 41 ready-made programs that cover every feature below: arithmetic and precedence, booleans and short-circuiting, currying, composition, recursion, ranges, `zip`, `filter`, list recursion (length, sum, reverse, quicksort), runtime errors and an infinite loop. Long programs are laid out over several lines.
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

filter (\x -> x % 2 == 0) [1..10]

let rec fact = \n -> if n == 0 then 1 else n * fact (n - 1) in fact 4
```

| Construct | Syntax |
|---|---|
| Numbers, booleans | `42`, `-3`, `true`, `false` (64-bit integers; overflow is an error) |
| Lists | `[1, 2, 3]`, `[]` |
| Ranges | `[1..5]` (inclusive; empty if the start is past the end; at most 10000 elements) |
| Functions | `\x -> x + 1`, `\x y -> x + y` (curried) |
| Application | `f x y` |
| Composition | `f . g` means `\x -> f (g x)` |
| Let | `let x = 1 in x + 1`, `let rec f = \n -> ... in f 3` |
| Conditionals | `if c then a else b` |
| Arithmetic | `+ - * / % ^` and unary minus `-x` |
| Comparison and logic | `< > <= >= == !=`, `&& \|\|` (`&&` and `\|\|` short-circuit) |
| List operators | `x : xs` (cons), `xs ++ ys` (append) |
| List functions | `head xs`, `tail xs`, `isEmpty xs`, `length xs` |
| Boolean not | `not b` |
| Map | `map f xs` |
| Fold | `fold f acc xs` (a left fold) |
| Filter | `filter p xs` (keeps the elements for which `p` returns `true`) |
| Zip | `zip xs ys` (a list of two-element lists; stops at the shorter list) |

### Operators

`^` is power (right-associative, whole-number exponents of 0 or more). `%` is the remainder with the sign of the divisor, like Haskell's `mod`. `/` is integer division and rounds toward zero. `==` and `!=` compare two numbers or two booleans.

From tightest to loosest:

| Level | Operators |
|---|---|
| 1 | function application, `f x` |
| 2 | `.` (right-associative) |
| 3 | `^` (right-associative) |
| 4 | `:` (right-associative) |
| 5 | `* / %` |
| 6 | `++` |
| 7 | `+ -` |
| 8 | `< > <= >= == !=` |
| 9 | `&&` |
| 10 | `\|\|` |

Two things differ from Haskell, so use parentheses when in doubt:

- `:` binds tighter than `*` and `+`. Write `(x * 2) : rest`, because `x * 2 : rest` means `x * (2 : rest)`.
- Unary minus applies to the application next to it, so `-x ^ 2` means `(-x) ^ 2`. Write `-(x ^ 2)` for the other reading.

### Keywords and built-ins

- `map`, `fold`, `filter` and `zip` are keywords that take their arguments directly, so wrap compound arguments in parentheses: `fold f 0 (map g xs)`.
- `head`, `tail`, `isEmpty`, `length` and `not` are built-in functions, but they are not first-class values. Apply them directly (`head xs`) and wrap them in a lambda to pass them around: `map (\xs -> isEmpty xs) lists`, not `map isEmpty lists`.

Evaluation is strict (call-by-value), left to right, using substitution. There are no strings, no tuples (a pair is a two-element list) and no pattern matching.

### Reduction rules

The badge on each step names the rule that fired.

| Rule | What happens |
|---|---|
| `let`, `letrec`, `unroll` | bind a value; unfold a recursive definition |
| `beta` | call a function: substitute the argument into the body |
| `if-true`, `if-false` | pick a branch |
| `arith` | compute an arithmetic or comparison result (including unary minus) |
| `short-circuit`, `and-or` | `&&` and `\|\|` decide early, or return their right side |
| `not` | flip a boolean |
| `list-cons`, `list-append` | build lists with `:` and `++` |
| `list-head`, `list-tail`, `list-empty`, `list-length` | the list functions |
| `range` | expand `[a..b]` into a list |
| `map-expand` | turn `map f xs` into one call per element |
| `fold-step`, `fold-done` | feed the next element into the accumulator; return it when the list is empty |
| `filter-start`, `filter-keep`, `filter-drop`, `filter-done` | test each element in turn, then return the kept ones |
| `zip` | pair up the two lists |
| `error`, `stopped`, `done` | a runtime error, the step limit, or the final value |

### Runtime errors

Karee stops at the offending subexpression for: division or modulo by zero, integer overflow, a negative exponent, `head` or `tail` of an empty list, an unbound variable, calling something that is not a function, and type mismatches such as `1 + true`.

## How it works

```
source text ──lex──▶ tokens ──parse──▶ AST ──step()──▶ AST ──step()──▶ ... ──▶ value
                                         │
                       each step also records: redex path, rule, caption
                                         ▼
                      pretty-printer ─▶ text + redex span      (Code view)
                      boxes.cpp      ─▶ list / map / fold / filter boxes
                      tree.cpp       ─▶ AST with the redex marked (Tree view)
                                         ▼
                              one JSON document, all steps
                                         ▼
                           browser: Player + renderers
```

- **`step(expr)`** (`src/eval.cpp`) finds the leftmost-innermost redex, rewrites it, and returns the new expression plus the *path* to the redex (a list of child indices). Everything visual is derived from that path.
- **Immutable ASTs.** Nodes are shared and never changed, so keeping every step costs little and stepping backward is free.
- **One run, many steps.** `run_program` evaluates to completion (or to the 5000-step limit) and returns every step as JSON. The browser's `Player` just walks that array.
- **Desugaring.** The parser rewrites `f . g` into a lambda, so composition needs no evaluation rule of its own.
- **`filter` as a state machine.** A running `filter` carries the remaining elements, the kept ones and the decisions made so far, which is what lets the boxes show kept and dropped elements.

### Layout

```
src/
  ast.h        expression type, spans, paths, operator precedence
  lexer.*      tokens and spans
  parser.*     recursive-descent parser
  eval.*       is_value, substitution, small-step reducer
  printer.*    expression -> text, and the redex's text span
  boxes.*      JSON for the list / map / fold / filter boxes
  tree.*       JSON for the syntax-tree view
  api.*        run_program: source -> JSON of all steps
  wasm_bind.cpp  exports run() to JavaScript
  main.cpp     command-line trace
tests/         parse, printer, eval and boxes tests
examples/      sample programs for the command line
web/
  index.html, style.css
  app.js       UI, program library, rendering
  player.js    timeline state
  tree.js      SVG tree with zoom, pan, Fit and Follow
  examples.js  the example gallery
build_wasm.sh
```

## Limits

- Runs stop after 5000 steps, which catches infinite loops, and the UI says so.
- Programs you write live in memory and are lost on reload. Share a link to keep one.