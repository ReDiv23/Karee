class Player {
    constructor() {
        this.steps = [];
        this.i = 0;
    }
    load(steps) {
        this.steps = steps;
        this.i = 0;
    }
    get count() {
        return this.steps.length;
    }
    get current() {
        return this.steps[this.i] || null;
    }

    get atStart() {
        return this.i === 0;
    }
    get atEnd() {
        return this.i >= this.steps.length - 1;
    }
    goto(n) {
        this.i = Math.max(0, Math.min(n, this.steps.length - 1));
        return this.current;
    }
    next() {
        return this.goto(this.i + 1);
    }
    prev() {
        return this.goto(this.i - 1);
    }
}

function splitAroundRedex(text, redex) {
    const [s, e] = redex;
    if (s < 0 || e <= s || e > text.length) return { before: text, mid: '', after: '' };
    return { before: text.slice(0, s), mid: text.slice(s, e), after: text.slice(e) };
}

if (typeof module !== 'undefined') module.exports = { Player, splitAroundRedex };