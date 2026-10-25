const EXAMPLES = [
    { name: "Let and arithmetic", code: "let x = 2 + 3 in x * x" },
    { name: "Map: double each box", code: "map (\\x -> x * 2) [1, 2, 3]" },
    { name: "Fold: sum a list", code: "fold (\\acc x -> acc + x) 0 [1, 2, 3, 4]" },
    { name: "Map feeding fold", code: "fold (\\a x -> a + x * x) 0 (map (\\x -> x + 1) [1, 2, 3])" },
    { name: "Factorial (let rec)", code: "let rec fact = \\n -> if n == 0 then 1 else n * fact (n - 1) in fact 4" },
    { name: "Currying and closures", code: "let add = \\x y -> x + y in\nlet inc = add 1 in\nmap inc [10, 20, 30]" },
    { name: "Fibonacci", code: "let rec fib = \\n -> if n < 2 then n else fib (n - 1) + fib (n - 2) in fib 5" },
    { name: "Powers: square each box", code: "map (\\x -> x ^ 2) [1, 2, 3, 4]" },
    { name: "GCD (let rec and %)", code: "let rec gcd = \\a b -> if b == 0 then a else gcd b (a % b) in gcd 48 18" },
    { name: "Runtime error", code: "map (\\x -> 10 / x) [5, 2, 0, 1]" },
    { name: "Infinite loop (stopped)", code: "let rec loop = \\n -> loop n in loop 1" },
];
if (typeof module !== 'undefined') module.exports = { EXAMPLES };