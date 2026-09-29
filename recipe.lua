-- How to build vetochka: the libraries it vendors from `mine` (check them out next to vetochka, as
-- the README shows), then vetochka itself.
return {
    muh_build = "0.1",
    requires = {
        { repo = "stb_ds-0.67",  as = "stb_ds", rev = "working" },
        { repo = "nob_da-3.8.2", as = "da",     rev = "working" },
        { repo = "arena",        as = "arena",  rev = "working" },
        { repo = "lua-5.5.1",    as = "lua",    rev = "working" },
    },

    run = function(repo)
        local stb_ds = repo.build "stb_ds"
        local da     = repo.build "da"
        local arena  = repo.build "arena"
        local lua    = repo.build "lua"
        return repo.project { deps = { stb_ds, da, arena, lua } }
    end,
}
