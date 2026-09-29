-- Ported from tests/zig/test_reducer.zig: cell nodes, their encoding, and the reducer.
local check = ...
local vetochka = require "vetochka"
local C = vetochka.cells
local T = C.type

local function append_node(cells, cursor, node)
    local index = assert(cells:alloc(node.size))
    check(index, cursor, "allocated index")
    assert(cells:write(index, node))
    return cursor + node.size
end

local function alloc_write(cells, node)
    local index = assert(cells:alloc(node.size))
    assert(cells:write(index, node))
    return index
end

--- Push `$ redex arg` and step to the end; the result index.
local function reduce(reducer, redex, arg)
    reducer:push("apply")
    reducer:push(redex)
    reducer:push(arg)
    repeat
        local code = reducer:step()
        assert(code >= 0, "reducer error " .. code .. ": " .. tostring(reducer:error()))
    until code == C.REDUCER_DONE
    check(reducer:has_result(), true, "has result")
    return reducer:result()
end

-- Trees for cells:build
local function D0() return { node = C.new_delta0() } end
local function D1(x) return { node = C.new_delta1(), x } end
local function D2(a, b) return { node = C.new_delta2(), a, b } end

local function expect_at(cells, index, want)
    local _, node = assert(cells:deref(index))
    check(node.type, want, "result type")
end
local function expect_left(cells, index, want)
    local _, node = assert(cells:left(index))
    check(node.type, want, "left type")
end
local function expect_right(cells, index, want)
    local i, node = assert(cells:right(index))
    check(node.type, want, "right type")
    return i
end

return {
    ["node info projections"] = function()
        for raw = 0, 255 do
            if C.type_valid_raw(raw) and raw ~= T.INVALID then
                assert(C.type_encodable(raw), "type " .. raw .. " is not encodable")
            end
        end
        check(C.type_arity(T.VALUEV1), 1, "arity of VALUEV1")
        check(C.type_with_arity(T.VALUEV1, 2), T.VALUEV2, "VALUEV1 with arity 2")
        check(C.type_arity(T.OP_FN1), 1, "arity of OP_FN1")
        check(C.type_with_arity(T.OP_FN1, 0), T.OP_FN0, "OP_FN1 with arity 0")
        check(C.type_with_arity(T.OP_FN1, 2), T.OP_FN2, "OP_FN1 with arity 2")

        check(C.new_node(T.DELTA0).type, T.DELTA0, "new DELTA0")
        check(C.new_node(T.VALUEV1).type, T.INVALID, "new_node of a payload type")

        local value = C.new_value0v("\xCA\xFE")
        local ok, value2 = C.set_arity(value, 2)
        check(ok, true, "set_arity on a value")
        check(value2.type, T.VALUEV2, "value with arity 2")
        check(value2.size, value.size, "encoded size kept")
        check((C.set_arity(C.new_ref(0), 0)), false, "set_arity on a ref")
    end,

    ["stable node byte encoding"] = function()
        local cells = assert(C.create(64))
        local cursor = 0
        for _, node in ipairs {
            C.new_delta0(), C.new_delta1(), C.new_delta2(),
            C.new_value0f(0x0102030405060708), C.new_value1f(0), C.new_value2f(-1),
            C.new_value0v(""), C.new_value1v(""), C.new_value2v("\xAA\xBB"),
            C.new_node(T.OP_FN0), C.new_node(T.OP_FN1), C.new_node(T.OP_FN2),
        } do
            cursor = append_node(cells, cursor, node)
        end
        local short_ref_index = cursor
        cursor = append_node(cells, cursor, C.new_ref(0x0123))
        local long_ref_index = cursor
        cursor = append_node(cells, cursor, C.new_ref(0x2000))

        local expected = string.char(
            0x80, 0x81, 0x82,
            0x83, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
            0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x85, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x86, 0x00, 0x87, 0x00, 0x88, 0x02, 0xAA, 0xBB,
            0x8A, 0x8B, 0x8C,
            0x01, 0x23, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00)
        check(cells:span():sub(1, cursor), expected, "encoded bytes")

        local short = cells:header(short_ref_index)
        check(short.type, T.REF, "short ref type")
        check(short.size, 2, "short ref size")
        check(cells:get(short_ref_index).ref, 0x0123, "short ref")
        local long = cells:header(long_ref_index)
        check(long.type, T.REF, "long ref type")
        check(long.size, 8, "long ref size")
        check(cells:get(long_ref_index).ref, 0x2000, "long ref")

        check(C.new_ref(math.maxinteger).type, T.INVALID, "a ref too far")
    end,

    ["smoke memory"] = function()
        local cells = assert(C.create(64))
        local tree = alloc_write(cells, C.new_delta0())
        local lhs = alloc_write(cells, C.new_ref(8191))
        local rhs = alloc_write(cells, C.new_value0f(-3516))
        check(cells:header(tree).type, T.DELTA0, "tree header")
        check(cells:header(lhs).type, T.REF, "lhs header")
        check(cells:header(rhs).type, T.VALUEF0, "rhs header")
        check(cells:get(tree).type, T.DELTA0, "tree node")
        check(cells:get(lhs).type, T.REF, "lhs node")
        check(cells:get(rhs).type, T.VALUEF0, "rhs node")
        for _, index in ipairs { tree, lhs, rhs } do
            assert(cells:free(index, cells:header(index).size))
        end
        for _, index in ipairs { tree, lhs, rhs } do
            check(cells:header(index).type, T.INVALID, "freed header")
        end
    end,

    ["debug view demo"] = function()
        local cells = assert(C.create(128))
        check(cells:header(alloc_write(cells, C.new_delta2())).type, T.DELTA2, "delta2")
        check(cells:header(alloc_write(cells, C.new_value0v("\xDE\xAD\xBE\xEF\xCA\xFE\xBA\xBE"))).type,
            T.VALUEV0, "value")
        check(cells:header(alloc_write(cells, C.new_ref(1024))).type, T.REF, "ref")
    end,

    ["opcode follows leaf stem fork saturation"] = function()
        local cells = assert(C.create(128))
        local reducer = assert(cells:reducer())
        local opcode = alloc_write(cells, C.new_node(T.OP_FN0))
        local first = alloc_write(cells, C.new_value0f(1))

        local stem = reduce(reducer, opcode, first)
        check(cells:header(stem).type, T.OP_FN1, "stem")
        local _, stem_left = assert(cells:left(stem))
        check(stem_left.type, T.VALUEF0, "stem's left type")
        check(stem_left.f, 1, "stem's left")

        reducer:reset()
        local second = alloc_write(cells, C.new_value0f(2))
        local fork = reduce(reducer, stem, second)
        check(cells:header(fork).type, T.OP_FN2, "fork")
        local _, fork_right = assert(cells:right(fork))
        check(fork_right.type, T.VALUEF0, "fork's right type")
        check(fork_right.f, 2, "fork's right")
    end,

    ["eval smoke"] = function()
        local cells = assert(C.create(256))
        local reducer = assert(cells:reducer())

        -- rule 0.a
        local r = reduce(reducer, alloc_write(cells, C.new_delta0()), alloc_write(cells, C.new_delta0()))
        expect_at(cells, r, T.DELTA1)
        expect_left(cells, r, T.DELTA0)

        -- rule 0.b: delta1 and its delta0 in one chunk
        local d1, d0 = C.new_delta1(), C.new_delta0()
        local stem = assert(cells:alloc(d1.size + d0.size))
        assert(cells:write(stem, d1))
        assert(cells:write(stem + d1.size, d0))
        r = reduce(reducer, stem, alloc_write(cells, C.new_delta0()))
        expect_at(cells, r, T.DELTA2)
        expect_left(cells, r, T.DELTA0)
        expect_right(cells, r, T.DELTA0)

        -- rule 1
        r = reduce(reducer, cells:build(D2(D0(), D2(D0(), D0()))), cells:build(D2(D0(), D0())))
        expect_at(cells, r, T.DELTA2)
        expect_left(cells, r, T.DELTA0)
        expect_right(cells, r, T.DELTA0)

        -- rule 2
        r = reduce(reducer, cells:build(D2(D1(D0()), D0())), cells:build(D0()))
        expect_at(cells, r, T.DELTA2)
        expect_left(cells, r, T.DELTA0)
        local right = expect_right(cells, r, T.DELTA1)
        expect_left(cells, right, T.DELTA0)

        -- rule 3a
        r = reduce(reducer, cells:build(D2(D2(D0(), D0()), D0())), cells:build(D0()))
        expect_at(cells, r, T.DELTA0)

        -- rule 3b
        r = reduce(reducer, cells:build(D2(D2(D0(), D0()), D0())), cells:build(D1(D0())))
        expect_at(cells, r, T.DELTA1)
        expect_left(cells, r, T.DELTA0)

        -- rule 3c
        r = reduce(reducer, cells:build(D2(D2(D0(), D0()), D0())), cells:build(D2(D0(), D0())))
        expect_at(cells, r, T.DELTA2)
        expect_left(cells, r, T.DELTA0)
        expect_right(cells, r, T.DELTA0)

        -- not, true, false
        local not_program = cells:build(D2(D2(D1(D0()), D2(D0(), D0())), D0()))
        local false_ = cells:build(D0())
        local true_ = cells:build(D1(D0()))
        r = reduce(reducer, not_program, false_) -- not false => true
        expect_at(cells, r, T.DELTA1)
        expect_left(cells, r, T.DELTA0)
        r = reduce(reducer, not_program, true_)  -- not true => false
        expect_at(cells, r, T.DELTA0)
    end,
}
