-- How to build vetochka's own files. What it builds against comes from recipe.lua.

local COMMON = { "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", "-fvisibility=hidden", "-DMUH_BUILDING" }

local MODES = {
    debug   = { cflags = { "-g", "-O0", "-fno-omit-frame-pointer", "-fsanitize=address,undefined" },
                ldflags = { "-fsanitize=address,undefined" } },
    release = { cflags = { "-g", "-O2" }, ldflags = {} },
}

--- One command line from words and lists of words.
local function command(...)
    local words = {}
    for _, part in ipairs { ... } do
        if type(part) == "table" then
            for _, w in ipairs(part) do words[#words + 1] = w end
        else
            words[#words + 1] = part
        end
    end
    return table.concat(words, " ")
end

return {
    muh_build = "0.1",
    mode      = "debug",
    -- link order: users before what they use (found from the objects' symbols)
    packages  = { "lua-binding", "reducer", "bytecode", "cells", "source", "allocator", "domain", "headeronly" },

    compile_cmd = function(out, src, extra_args, m)
        return command("clang", COMMON, MODES[m.mode].cflags, extra_args, "-MMD", "-MF", out .. ".d", "-c", src, "-o", out)
    end,
    archive_cmd = function(out, ins)
        return command("ar", "rcs", out, ins)
    end,
    link_cmd = function(out, ins, extra_args, m)
        return command("clang", MODES[m.mode].ldflags, ins, "-o", out, extra_args or {})
    end,
}
