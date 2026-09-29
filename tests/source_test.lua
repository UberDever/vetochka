-- Ported from tests/zig/test_source.zig.
local check = ...
local vetochka = require "vetochka"

local function parses(text)
    local tree, err = vetochka.parse(text)
    assert(tree, "parse failed: " .. tostring(err) .. "\n" .. text)
    check(tree:sub(1, 7), "(source", "tree root")
end

local function rejects(text)
    local tree = vetochka.parse(text)
    assert(tree == nil, "parsed, but should be rejected:\n" .. text)
end

return {
    ["source grammar accepts the spec forms"] = function()
        for _, text in ipairs {
            "",
            "0; 42; {text {nested}}; name?; foo'; x-y",
            "~[]; ~[x]; ~[x, y]; ~ [x]",
            "@[doc, more] !value.field(1, 2,)[index]{payload}",
            "[\n  (grouped),\n  item,\n]\nf()\nleft :: right",
            "call label: -value other: @[meta] target",
            "task do first; nested do value end; last end",
            "x: 1; do a end; [x: 2, y: 4]; (x: (y: 1))",
            "f do: 1 end: 2",
            "$fn: [x] do x end to: 42; $ fn: [] do 1 end",
            "$ns: {core} op: {add} x: 1 y: 2",
            "a + b * c - d; !!x; -(x); x - y; a ^ b; ^x",
            "f(x)(y).z{bytes}[w]",
            "first\nsecond ...\n  + third\n;; line comment\n#| outer #| nested |# comment |#\n#; discarded(1, 2)\nlast",
            -- layout: parens and brackets are inactive, a nested do-block is active again
            "f(a\n  + b)\ng(do\n  one\n  two\nend)",
        } do
            parses(text)
        end
    end,

    ["source grammar rejects what the spec forbids"] = function()
        for _, text in ipairs {
            "x -y",       -- prefix operator after an operand: no juxtaposition
            "1-2",        -- operator glued to a literal
            "f(x)-y",     -- operator glued to `)`
            "x=",         -- operator glued to an identifier
            "f (a)",      -- tight postfix must be glued
            "f x",        -- no juxtaposition
            "x : y",      -- free lone colon
            "a . b",      -- free dot
            "~[1, 2, 3]", -- nyads have at most two children
            "~[1,]",
            "$",          -- `$` must head a labeled expression
            "$ x",
            "@x",         -- `@` only opens `@[`
            "^",          -- an operator with no operands
            "\u{394}",    -- Δ: the lexer is ASCII-only for now, task 20260924-123410
            "007",
            "@[doc]\ntarget", -- annotation `]` ends an expression: ASI inserts `;`
        } do
            rejects(text)
        end
    end,

    ["source formatters smoke"] = function()
        local text = "@[trace] run args: [1, 2,] do\n  value.field()\n  result = !value + 1\n  $fn: [x] do x end\nend"
        local canonical = assert(vetochka.format(text))
        for _, part in ipairs { "@[trace]", "value.field()", "!value + 1", "$fn: [x]" } do
            assert(canonical:find(part, 1, true), "canonical form lacks " .. part .. ":\n" .. canonical)
        end
        -- The canonical form parses again.
        assert(vetochka.parse(canonical))

        local sexpr = assert(vetochka.parse(text))
        check(sexpr:sub(1, 7), "(source", "s-expression root")
        assert(sexpr:find("(annotation", 1, true), "no (annotation")
        assert(sexpr:find("(op-prefix", 1, true), "no (op-prefix")
    end,
}
