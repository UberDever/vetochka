-- How to build vetochka in its workspace (~/dev/vetochka-repo): the header-only libraries it
-- includes, then vetochka itself.
return {
    muh_build = "0.1",
    requires = {
        { repo = "stb_ds-0.67",  as = "stb_ds", rev = "working" },
        { repo = "nob_da-3.8.2", as = "da",     rev = "working" },
        { repo = "arena",        as = "arena",  rev = "working" },
    },

    run = function(repo)
        local stb_ds = repo.build "stb_ds"
        local da     = repo.build "da"
        local arena  = repo.build "arena"
        return repo.project { deps = { stb_ds, da, arena } }
    end,
}
