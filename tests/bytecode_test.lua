-- Ported from tests/zig/test_bytecode.zig: source text lowered to cells, checked node by node.
local check = ...
local vetochka = require "vetochka"
local T = vetochka.cells.type

-- A located node: {cells, index, node}.
local function node_at(cells, index)
    local header = cells:header(index)
    assert(header.type ~= T.INVALID, "invalid node at " .. index)
    return { cells = cells, index = index, node = cells:get(index) }
end

local function left(n)
    local index, node = assert(n.cells:left(n.index))
    return { cells = n.cells, index = index, node = node }
end

local function right(n)
    local index, node = assert(n.cells:right(n.index))
    return { cells = n.cells, index = index, node = node }
end

local function expect_type(n, want) check(n.node.type, want, "node type") end

local function expect_bytes(n, want)
    expect_type(n, T.VALUEV0)
    check(n.node.v, want, "bytes")
end

local function expect_int(n, want)
    expect_type(n, T.VALUEF0)
    check(n.node.f, want, "integer")
end

local function expect_nil(n) expect_type(n, T.DELTA0) end

--- Proper list ~[a, ~[b, ~[]]]: its items, checking the ~[] terminator.
local function items(list)
    local out, cursor = {}, list
    while cursor.node.type == T.DELTA2 do
        out[#out + 1] = left(cursor)
        cursor = right(cursor)
    end
    expect_nil(cursor)
    return out
end

--- Tagged node [{tag}, meta, ...fields] with empty meta: its fields.
local function tagged(n, tag)
    local all = items(n)
    assert(#all >= 2, "tagged node too short")
    expect_bytes(all[1], tag)
    expect_nil(all[2])
    return { table.unpack(all, 3) }
end

local function expect_id(n, name)
    local fields = tagged(n, ":id")
    check(#fields, 1, "id fields")
    expect_bytes(fields[1], name)
end

--- [{@}, meta, f, x]: f and x.
local function application(n)
    local fields = tagged(n, "@")
    check(#fields, 2, "application fields")
    return fields[1], fields[2]
end

local function encode(text, options)
    local cells, root = assert(vetochka.encode(text, options))
    return node_at(cells, root)
end

--- The single statement of a one-statement source block.
local function statement(text)
    local body = tagged(encode(text), ":block")
    check(#body, 1, "statements")
    return body[1]
end

return {
    ["rule 1: identifiers"] = function()
        expect_id(statement("x"), "x")
    end,

    ["literals lower to themselves"] = function()
        local list = items(statement("[42, {text {nested}}]"))
        check(#list, 2, "items")
        expect_int(list[1], 42)
        expect_bytes(list[2], "text {nested}")
    end,

    ["rules 3 and 4: lists are plain nyad lists, parens are erased"] = function()
        local list = items(statement("[(x), 1]"))
        check(#list, 2, "items")
        expect_id(list[1], "x")
        expect_int(list[2], 1)
    end,

    ["nyads"] = function()
        local list = items(statement("[~[], ~[1], ~[1, 2]]"))
        expect_nil(list[1])
        expect_type(list[2], T.DELTA1)
        expect_int(left(list[2]), 1)
        expect_type(list[3], T.DELTA2)
        expect_int(right(list[3]), 2)
    end,

    ["rule 5: applications are {@} data, curried"] = function()
        local f1, x2 = application(statement("f(1, 2)"))
        expect_int(x2, 2)
        local f, x1 = application(f1)
        expect_id(f, "f")
        expect_int(x1, 1)
    end,

    ["f() is f(~[])"] = function()
        local _, x = application(statement("f()"))
        expect_nil(x)
    end,

    ["rules 9 and 10: f[...] and f{...}"] = function()
        local inner, b = application(statement("f[1]{b}"))
        expect_bytes(b, "b")
        local _, list = application(inner)
        expect_int(items(list)[1], 1)
    end,

    ["rule 6 and loose postfix: labels"] = function()
        local f, label = application(statement("f x: 1"))
        expect_id(f, "f")
        local fields = tagged(label, ":label")
        expect_bytes(fields[1], "x")
        expect_int(fields[2], 1)
    end,

    ["rule 7: blocks, as a loose postfix"] = function()
        local _, block = application(statement("f do 1; 2 end"))
        local body = tagged(block, ":block")
        check(#body, 2, "block statements")
        expect_int(body[2], 2)
    end,

    ["rule 8: annotations"] = function()
        local annot = tagged(statement("@[a, b] x"), ":annot")
        check(#items(annot[1]), 2, "annotation items")
        expect_id(annot[2], "x")
    end,

    ["rule 11: prefix operators"] = function()
        local prefix = tagged(statement("-x"), ":prefix")
        expect_bytes(prefix[1], "-")
        expect_id(prefix[2], "x")
    end,

    ["rule 12: mixed infix chains are flat, operators in a list"] = function()
        local infix = tagged(statement("a + b * c"), ":infix")
        check(#infix, 4, "infix fields")
        local ops = items(infix[1])
        expect_bytes(ops[1], "+")
        expect_bytes(ops[2], "*")
        expect_id(infix[4], "c")
    end,

    ["rule 12: parens keep grouping as nesting"] = function()
        local infix = tagged(statement("(a + b) * c"), ":infix")
        check(#infix, 3, "infix fields")
        check(#tagged(infix[2], ":infix"), 3, "inner infix fields")
    end,

    ["rule 13: selectors"] = function()
        local sel = tagged(statement("base.name"), ":selector")
        expect_id(sel[1], "base")
        expect_bytes(sel[2], "name")
    end,

    ["rule 2: $ heads an opcode as plain application"] = function()
        local head, label = application(statement("$fn: [x]"))
        expect_id(head, "$")
        expect_bytes(tagged(label, ":label")[1], "fn")
    end,

    ["source positions in meta when asked"] = function()
        local block = items(encode("\n  x", { source_meta = true }))
        local id = items(block[3])
        expect_bytes(id[1], ":id")
        local meta = items(id[2])
        check(#meta, 2, "meta entries")
        local line = tagged(meta[1], ":label")
        expect_bytes(line[1], "line")
        expect_int(line[2], 2)
        expect_int(tagged(meta[2], ":label")[2], 3)
    end,
}
