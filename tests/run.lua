-- run.lua — runs test files:  vtest tests/run.lua tests/*_test.lua
--
-- A test file gets `check` as its argument (`local check = ...`) and returns a table of named test
-- functions. A test fails by raising an error; check(got, want, what) raises one when got ~= want.

local function check(got, want, what)
    if got ~= want then
        error(string.format("%s: got %q, want %q", what or "value", tostring(got), tostring(want)), 2)
    end
end

local passed, failed = 0, 0
for _, file in ipairs(arg) do
    local tests = assert(loadfile(file))(check)
    local names = {}
    for name in pairs(tests) do names[#names + 1] = name end
    table.sort(names)
    for _, name in ipairs(names) do
        local ok, err = pcall(tests[name])
        if ok then
            passed = passed + 1
        else
            failed = failed + 1
            print(string.format("FAIL %s: %s\n  %s", file, name, tostring(err)))
        end
    end
end
print(string.format("%d passed, %d failed", passed, failed))
return failed == 0 and 0 or 1
