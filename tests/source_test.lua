local check = ...
local vetochka = require "vetochka"

return {
    ["the empty nyad parses into a source tree"] = function()
        local tree = assert(vetochka.parse("~[]"))
        check(tree:sub(1, 7), "(source", "tree root")
    end,

    ["malformed text is refused with a message"] = function()
        local tree, err = vetochka.parse("~[")
        check(tree, nil, "tree")
        check(err, "parse error", "message")
    end,

    ["canonical form parses again"] = function()
        local canonical = assert(vetochka.format("f(x,   y)"))
        check(canonical, "f(x, y)\n", "canonical form")
        assert(vetochka.parse(canonical))
    end,
}
