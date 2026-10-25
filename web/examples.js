const program = (strings) => {
    const lines = strings.raw[0].replace(/^\n/, '').replace(/\s+$/, '').split('\n');
    const indent = Math.min(...lines.filter((l) => l.trim()).map((l) => l.match(/^ */)[0].length));
    return lines.map((l) => l.slice(indent)).join('\n');
};

const EXAMPLES = [
    {
        name: "Let and arithmetic",
        code: program`
            let x = 2 + 3 in x * x
        `,
    },
    {
        name: "Map: double each box",
        code: program`
            map (\x -> x * 2) [1, 2, 3]
        `,
    },
    {
        name: "Fold: sum a list",
        code: program`
            fold (\acc x -> acc + x) 0 [1, 2, 3, 4]
        `,
    },
    {
        name: "Map feeding fold",
        code: program`
            fold (\a x -> a + x * x) 0
                 (map (\x -> x + 1) [1, 2, 3])
        `,
    },
    {
        name: "Comments",
        code: program`
            -- double a number, then add one
            let f = \x -> x * 2 + 1 in
            f 10    -- the result is 21
        `,
    },

    {
        name: "Precedence: ^ then * then +",
        code: program`
            -- ^ binds tighter than *, and * tighter than + and -
            2 + 3 * 4 ^ 2 - 10 / 5
        `,
    },
    {
        name: "Powers: square each box",
        code: program`
            map (\x -> x ^ 2) [1, 2, 3, 4]
        `,
    },
    {
        name: "Division and remainder",
        code: program`
            let a = 17 in
            (a / 5) * 5 + a % 5
        `,
    },
    {
        name: "Comparisons and booleans",
        code: program`
            let x = 7 in
            let small = x < 3 in
            let medium = x >= 3 && x != 5 in
            small || medium
        `,
    },
    {
        name: "Short-circuit: no division by zero",
        code: program`
            -- && already knows the answer is false, so the right side never runs
            false && (1 / 0 == 1)
        `,
    },
    {
        name: "Unary minus: negate each box",
        code: program`
            map (\x -> -x) [1, -2, 3]
        `,
    },
    {
        name: "Boolean not: flip each test",
        code: program`
            map (\x -> not (x > 2)) [1, 2, 3, 4]
        `,
    },
    {
        name: "If and else: sign of a number",
        code: program`
            let sign = \n ->
                if n < 0 then -1
                else if n == 0 then 0
                else 1
            in
            map sign [-5, 0, 8]
        `,
    },

    {
        name: "Currying and closures",
        code: program`
            let add = \x y -> x + y in
            let inc = add 1 in
            map inc [10, 20, 30]
        `,
    },
    {
        name: "Function composition (.)",
        code: program`
            let inc = \x -> x + 1 in
            let double = \x -> x * 2 in
            map (inc . double) [1, 2, 3]
        `,
    },
    {
        name: "Functions as arguments",
        code: program`
            let twice = \f x -> f (f x) in
            twice (\n -> n * 3) 2
        `,
    },

    {
        name: "Factorial (let rec)",
        code: program`
            let rec fact = \n ->
                if n == 0 then 1
                else n * fact (n - 1)
            in
            fact 4
        `,
    },
    {
        name: "Fibonacci",
        code: program`
            let rec fib = \n ->
                if n < 2 then n
                else fib (n - 1) + fib (n - 2)
            in
            fib 5
        `,
    },
    {
        name: "GCD (let rec and %)",
        code: program`
            let rec gcd = \a b ->
                if b == 0 then a
                else gcd b (a % b)
            in
            gcd 48 18
        `,
    },

    {
        name: "Cons: build a list with :",
        code: program`
            1 : 2 : 3 : []
        `,
    },
    {
        name: "Append: join lists with ++",
        code: program`
            [1, 2] ++ [3, 4] ++ [5]
        `,
    },
    {
        name: "Head and tail",
        code: program`
            head (tail [10, 20, 30])
        `,
    },
    {
        name: "Empty list check",
        code: program`
            map (\xs -> isEmpty xs) [[], [1], [2, 3]]
        `,
    },
    {
        name: "List length",
        code: program`
            length [2, 4, 6, 8]
        `,
    },
    {
        name: "Range: [1..5]",
        code: program`
            map (\x -> x * x) [1..6]
        `,
    },
    {
        name: "Factorial from a range",
        code: program`
            fold (\acc x -> acc * x) 1 [1..5]
        `,
    },
    {
        name: "Zip two lists",
        code: program`
            zip [1, 2, 3] [10, 20, 30]
        `,
    },
    {
        name: "Zip, then multiply each pair",
        code: program`
            map (\p -> head p * head (tail p))
                (zip [1, 2, 3] [4, 5, 6])
        `,
    },

    {
        name: "Filter: keep the evens",
        code: program`
            filter (\x -> x % 2 == 0) [1, 2, 3, 4, 5, 6]
        `,
    },
    {
        name: "Filter a range",
        code: program`
            length (filter (\x -> x % 7 == 0) [1..21])
        `,
    },
    {
        name: "Filter, map and fold together",
        code: program`
            -- sum of the squares of the odd numbers up to 9
            fold (\acc x -> acc + x) 0
                 (map (\x -> x ^ 2)
                      (filter (\x -> x % 2 == 1) [1..9]))
        `,
    },

    {
        name: "Recursive length",
        code: program`
            let rec len = \xs ->
                if isEmpty xs then 0
                else 1 + len (tail xs)
            in
            len [5, 10, 15, 20]
        `,
    },
    {
        name: "Recursive sum (head and tail)",
        code: program`
            let rec total = \xs ->
                if isEmpty xs then 0
                else head xs + total (tail xs)
            in
            total [1, 2, 3, 4]
        `,
    },
    {
        name: "My own map (cons)",
        code: program`
            let rec double_all = \xs ->
                if isEmpty xs then []
                else (head xs * 2) : double_all (tail xs)
            in
            double_all [1, 2, 3]
        `,
    },
    {
        name: "Reverse (append)",
        code: program`
            let rec rev = \xs ->
                if isEmpty xs then []
                else rev (tail xs) ++ [head xs]
            in
            rev [1, 2, 3, 4]
        `,
    },
    {
        name: "Quicksort",
        code: program`
            let rec qsort = \xs ->
                if isEmpty xs then []
                else
                    let p = head xs in
                    let rest = tail xs in
                    qsort (filter (\x -> x < p) rest)
                        ++ [p]
                        ++ qsort (filter (\x -> x >= p) rest)
            in
            qsort [3, 1, 4, 1, 5]
        `,
    },

    {
        name: "Runtime error: division by zero",
        code: program`
            map (\x -> 10 / x) [5, 2, 0, 1]
        `,
    },
    {
        name: "Runtime error: head of an empty list",
        code: program`
            head (tail [1])
        `,
    },
    {
        name: "Runtime error: wrong type",
        code: program`
            1 + true
        `,
    },
    {
        name: "Overflow is an error",
        code: program`
            2 ^ 64
        `,
    },
    {
        name: "Infinite loop (stopped)",
        code: program`
            let rec loop = \n -> loop n in
            loop 1
        `,
    },
];
if (typeof module !== 'undefined') module.exports = { EXAMPLES };