# Artifact: collections/vector syntax experiment

- Original path: `project/vetochka_collections_vec.tree`
- Historical context date: 2026-05-28
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

```vetochka
module [collections.vec] do:
    import(std.mem);
    import(std.assert);

    ;; stuff
    struct [Vec, [T]] do:
        field(data, ptr(T));
        field(len, usize);
        field(cap, usize);
        field(alloc, ptr(Allocator));
    end;

    @[an annotation]
    fn [vec_init, [alloc :: ptr(Allocator)]] ret: Vec do:
        return(make(Vec, [
            data = null,
            len = 0,
            cap = 0,
            alloc = alloc,
        ]));
    end;

    fn [vec_free, [v :: ptr(Vec(T))]] do:
        if [v.data != null] do:
            free(v.alloc, v.data);
        end;

        v.data = null;
        v.len = 0;
        v.cap = 0;
    end;

    fn [vec_reserve, [v :: ptr(Vec(T)), wanted :: usize]] ret: bool do:
        if [wanted <= v.cap] do:
            return(true);
        let ~val doubled = v.cap * 2,
            ~val new_cap = max(wanted, max(doubled, 8));

        with [new_data = alloc_array(v.alloc, T, new_cap)] do:
            if [v.data != null] do:
                copy(new_data, v.data, v.len);
                free(v.alloc, v.data);
            end;

            v.data = new_data;
            v.cap = new_cap;

            return(true);
        else:
            return(false);
        end;
    end;

    fn [vec_push, [v :: ptr(Vec(T)), value :: T]] ret: bool do:
        if [v.len == v.cap] do:
            if [not(vec_reserve(v, v.len + 1))] do:
                return(false);
            else:
                v.data[v.len] = value;
                v.len = v.len + 1;
                return(true);
            end;
        else:
            v.data[v.len] = value;
            v.len = v.len + 1;
            return(true);
        end;
    end;

    fn [vec_get, [v :: ptr(Vec(T)), index :: usize]] ret: ptr(T) do:
        assert(index < v.len);
        v.data[index];
    end;

    fn [emit_vec_api, [T, Name]] ret: artifact do:
        emit_header [
            c_name = Name,
            decls = [
                fn_decl [concat(Name, {init}), [], Name],
                fn_decl [concat(Name, {free}), [ptr(Name)], void],
                fn_decl [concat(Name, {push}), [ptr(Name), T], bool],
                fn_decl [concat(Name, {get}), [ptr(Name), usize], ptr(T)],
            ],
        ];

        emit_source [
            c_name = Name,
            body = c {generated vector implementation goes here},
        ];

    end;
end

```
