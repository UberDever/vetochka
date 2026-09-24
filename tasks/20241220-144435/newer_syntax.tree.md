# Artifact: newer syntax experiment

- Original path: `project/newer_syntax.tree`
- Historical context date: 2026-06-11
- Status: historical design artifact; design hint only, not current spec.
- Preservation: exact original content follows.

---

```vetochka
def module: Example do
def fn: moving_average args: [values :: List[f64], window :: i64] ret: List[f64] do
    def var: result = List[f64]()
    for range: [i = u64{0}, values.length()] do
        def let: block do
            from = max(0, i - window + 1)
            slice = values[range(from, i + 1)]
        end var: sum = 0
        for each: [x, slice] do
            sum += x
        end
        result.append(sum / slice.length())
    end
    return(result)
end
print(moving_average ...
        ([1, 2, 3, 4, 5], 3))
print(^(^, ^)(^))
end


;; Specialized into the following

[{:block},
  {$}(
    {$}([{:id}, {def}], [{:label}, {module}, [{:id}, {Example}]]),
    [{:block},

      {$}(
        {$}(
          {$}(
            {$}([{:id}, {def}], [{:label}, {fn}, [{:id}, {moving_average}]]),
            [{:label}, {args}, [
              {$}({$}([{:infix}, {::}], [{:id}, {values}]),
                  {$}([{:id}, {List}], [[{:id}, {f64}]])),
              {$}({$}([{:infix}, {::}], [{:id}, {window}]), [{:id}, {i64}])
            ]]
          ),
          [{:label}, {ret}, {$}([{:id}, {List}], [[{:id}, {f64}]])]
        ),
        [{:block},

          {$}([{:id}, {def}],
              [{:label}, {var},
                {$}({$}([{:infix}, {=}], [{:id}, {result}]),
                    {$}({$}([{:id}, {List}], [[{:id}, {f64}]]), ^))
              ]),

          {$}(
            {$}([{:id}, {for}],
                [{:label}, {range}, [
                  {$}({$}([{:infix}, {=}], [{:id}, {i}]), {$}([{:id}, {u64}], 0)),
                  {$}({$}({$}({.}, [{:id}, {values}]), [{:id}, {length}]), ^)
                ]]),
            [{:block},

              {$}(
                {$}(
                  {$}([{:id}, {def}], [{:label}, {let}, [{:id}, {block}]]),
                  [{:block},
                    {$}({$}([{:infix}, {=}], [{:id}, {from}]),
                        {$}({$}([{:id}, {max}], 0),
                            {$}({$}([{:infix}, {+}],
                                    {$}({$}([{:infix}, {-}], [{:id}, {i}]),
                                        [{:id}, {window}])),
                                1))),
                    {$}({$}([{:infix}, {=}], [{:id}, {slice}]),
                        {$}([{:id}, {values}],
                            [{$}({$}([{:id}, {range}], [{:id}, {from}]),
                                 {$}({$}([{:infix}, {+}], [{:id}, {i}]), 1))]))
                  ]
                ),
                [{:label}, {var}, {$}({$}([{:infix}, {=}], [{:id}, {sum}]), 0)]
              ),

              {$}(
                {$}([{:id}, {for}],
                    [{:label}, {each}, [[{:id}, {x}], [{:id}, {slice}]]]),
                [{:block},
                  {$}({$}([{:infix}, {+=}], [{:id}, {sum}]), [{:id}, {x}])
                ]
              ),

              {$}(
                {$}({$}({.}, [{:id}, {result}]), [{:id}, {append}]),
                {$}({$}([{:infix}, {/}], [{:id}, {sum}]),
                    {$}({$}({$}({.}, [{:id}, {slice}]), [{:id}, {length}]), ^))
              )
            ]
          ),

          {$}([{:id}, {return}], [{:id}, {result}])
        ]
      ),

      {$}([{:id}, {print}],
          {$}({$}([{:id}, {moving_average}], [1, 2, 3, 4, 5]), 3)),

      {$}([{:id}, {print}], {$}({$}({$}(^, ^), ^), ^))
    ]
  )
]

```
