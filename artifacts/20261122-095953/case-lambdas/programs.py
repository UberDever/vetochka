"""v0 programs for the case-lambda experiment.

The program texts below are lowered v0 trees. To keep them readable, `$name` stands for the
identifier tree a vf parser would emit for that declaration and its occurrences:
`[[{:id}], {name}, ~[{:private}, ~{NN}]]`, with one parser-issued identity per distinct
`$name` in a program, numbered from 01 in order of first appearance. `$name.2` spells the same
name with a different identity. `$unique` is the host intrinsic, identity `~{f0}`.
`expand` performs only this textual substitution; `python3 programs.py` writes the expanded
programs to `programs/*.v0`.
"""

from pathlib import Path
import re

GLOBALS = {"unique": "f0"}
PLACEHOLDER = re.compile(r"\$([A-Za-z][A-Za-z0-9-]*)(\.[0-9]+)?")


def expand(template):
    issued = {}

    def identifier(match):
        name, variant = match.group(1), match.group(2) or ""
        if name in GLOBALS and not variant:
            key = GLOBALS[name]
        else:
            key = issued.setdefault(name + variant, f"{len(issued) + 1:02x}")
        return "[[{:id}], {" + name + "}, ~[{:private}, ~{" + key + "}]]"

    return PLACEHOLDER.sub(identifier, template)


PROGRAMS = {
    # A builder lambda receives one generated identifier and uses it for both the declaration
    # and the occurrence of the lambda it returns.
    "builder": """
[[{:binder}], [
  ~[$x, [[{$}], $unique, {x}]],
  ~[$mk, [[{:lambda}], ~, [[$decl], [$body]],
          ~[[{:all}, {:all}], [[{:lambda}], ~, [[$decl]], ~[[{:all}], $body]]]]],
  ~[$mk1, [[{$}], $mk, $x]],
  ~[$f, [[{$}], $mk1, $x]],
  ~[$r, [[{$}], $f, {hello}]]
 ], [$f, $r]]
""",
    # A case-lambda assembled from generated identifiers, applied partially once and then
    # completed three times with a fork, a fruit, and a leaf.
    "shapes": """
[[{:binder}], [
  ~[$a, [[{$}], $unique, {a}]],
  ~[$l, [[{$}], $unique, {l}]],
  ~[$r, [[{$}], $unique, {r}]],
  ~[$f, [[{:lambda}], ~, [[$a], [$l, $r]],
          ~[[{:all}, {:fork}], ~[$a, ~[$l, $r]]],
          ~[[{:all}, {:fruit}], ~[$a, $l]],
          ~[[{:all}, {:leaf}], $a]]],
  ~[$g, [[{$}], $f, {p}]],
  ~[$v1, [[{$}], $g, ~[{b}, {c}]]],
  ~[$v2, [[{$}], $g, {d}]],
  ~[$v3, [[{$}], $g, ~]]
 ], [$f, $g, $v1, $v2, $v3]]
""",
    # Two inspected positions: both rows survive the first argument; the second decides.
    "two-positions": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$h1, $t1], [$h2, $t2]],
          ~[[{:fork}, {:leaf}], {fl}],
          ~[[{:fork}, {:fruit}], ~[$t1, $h2]],
          ~[[{:all}, {:all}], {other}]]],
  ~[$g, [[{$}], $f, ~[{a}, {b}]]],
  ~[$v1, [[{$}], $g, ~]],
  ~[$v2, [[{$}], $g, {c}]],
  ~[$g2, [[{$}], $f, {x}]],
  ~[$v3, [[{$}], $g2, {c}]]
 ], [$v1, $v2, $v3]]
""",
    # Overlapping cases: the first matching row in written order wins.
    "overlap": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$h, $t]],
          ~[[{:fork}], {first}],
          ~[[{:all}], {any}],
          ~[[{:fork}], {shadowed}]]],
  ~[$v1, [[{$}], $f, ~[{1}, {2}]]],
  ~[$v2, [[{$}], $f, {z}]],
  ~[$v3, [[{$}], $f, ~]]
 ], [$v1, $v2, $v3]]
""",
    # A binder held as data inside a fork, extracted by a case-lambda, then run by a clause.
    "binder": """
[[{:binder}], [
  ~[$y, [[{$}], $unique, {y}]],
  ~[$z, [[{$}], $unique, {z}]],
  ~[$box, ~[{box}, [[{:binder}], [~[$y, {v}], ~[$z, ~[$y, $y]]], $z]]],
  ~[$unbox, [[{:lambda}], ~, [[$h, $t]], ~[[{:fork}], $t]]],
  ~[$b, [[{$}], $unbox, $box]],
  ~[$res, $b]
 ], [$b, $res]]
""",
    # Two generated identifiers with the same spelling stay distinct bindings.
    "identity": """
[[{:binder}], [
  ~[$x, [[{$}], $unique, {x}]],
  ~[$x.2, [[{$}], $unique, {x}]],
  ~[$f, [[{:lambda}], ~, [[$x]], ~[[{:all}], ~[$x, $x.2]]]],
  ~[$r, [[{$}], $f, {arg}]]
 ], $r]
""",
    # A parsed two-parameter lambda partially applied to identifier data. Its second
    # application must still see its own declarations, not the argument (22.11.26, 14).
    "identifier-argument": """
[[{:binder}], [
  ~[$i, [[{$}], $unique, {i}]],
  ~[$k, [[{:lambda}], ~, [[$p], [$q]],
          ~[[{:leaf}, {:all}], {leaf}],
          ~[[{:all}, {:all}], ~[$p, $q]]]],
  ~[$k1, [[{$}], $k, $i]],
  ~[$out, [[{$}], $k1, {w}]]
 ], $out]
""",
    # Shape inspection sees the rewritten tree: inside `f`, which declares the generated `X`,
    # the fork child `x` rewrites through `X` to the argument fruit, as execution does.
    "observe": """
[[{:binder}], [
  ~[$x, [[{$}], $unique, {x}]],
  ~[$kind, [[{:lambda}], ~, [[$a, $b]],
             ~[[{:leaf}], {leaf}], ~[[{:fork}], {fork}], ~[[{:fruit}], {fruit}]]],
  ~[$first, [[{:lambda}], ~, [[$h, $t]],
              ~[[{:fork}], [[{:binder}], [~[$o, [[{$}], $kind, $h]]], $o]]]],
  ~[$f, [[{:lambda}], ~, [[$x]],
          ~[[{:all}], [[{:binder}], [~[$o.2, [[{$}], $first, ~[$x, {k}]]]], $o.2]]]],
  ~[$r, [[{$}], $f, {arg}]]
 ], $r]
""",
    # Reflective re-entry: take `p`'s declaration out of `k` from outside, pass it back into
    # `k`, and use it as the declaration of a lambda built inside `k`, where `p` is bound.
    # `reflect` binds `p` to a fruit; `reflect-identifier` binds it to a generated identifier.
    "reflect": """
[[{:binder}], [
  ~[$k, [[{:lambda}], ~, [[$p], [$hdr]],
          ~[[{:all}, {:all}],
            [[{:binder}], [
              ~[$g, [[{:lambda}], ~, [[$hdr]], ~[[{:all}], $hdr]]],
              ~[$o, [[{$}], $g, {z}]]
            ], $o]]]],
  ~[$tl, [[{:lambda}], ~, [[$h, $t]], ~[[{:fork}], $t]]],
  ~[$hd, [[{:lambda}], ~, [[$h2, $t2]], ~[[{:fork}], $h2]]],
  ~[$s1, [[{$}], $tl, $k]],
  ~[$s2, [[{$}], $tl, $s1]],
  ~[$hl, [[{$}], $hd, $s2]],
  ~[$pos, [[{$}], $hd, $hl]],
  ~[$pid, [[{$}], $hd, $pos]],
  ~[$k1, [[{$}], $k, {arg}]],
  ~[$out, [[{$}], $k1, $pid]]
 ], [$pid, $out]]
""",
    "reflect-identifier": """
[[{:binder}], [
  ~[$i, [[{$}], $unique, {i}]],
  ~[$k, [[{:lambda}], ~, [[$p], [$hdr]],
          ~[[{:all}, {:all}],
            [[{:binder}], [
              ~[$g, [[{:lambda}], ~, [[$hdr]], ~[[{:all}], $hdr]]],
              ~[$o, [[{$}], $g, {z}]]
            ], $o]]]],
  ~[$tl, [[{:lambda}], ~, [[$h, $t]], ~[[{:fork}], $t]]],
  ~[$hd, [[{:lambda}], ~, [[$h2, $t2]], ~[[{:fork}], $h2]]],
  ~[$s1, [[{$}], $tl, $k]],
  ~[$s2, [[{$}], $tl, $s1]],
  ~[$hl, [[{$}], $hd, $s2]],
  ~[$pos, [[{$}], $hd, $hl]],
  ~[$pid, [[{$}], $hd, $pos]],
  ~[$k1, [[{$}], $k, $i]],
  ~[$out, [[{$}], $k1, $pid]]
 ], [$pid, $out]]
""",
    # Lists held in variables: a lambda whose case list is the variable `cases`, a two-parameter
    # lambda whose cases come from a variable and complete through a partial application, and
    # a binder (held as data, then forced) whose clause list is the variable `cl`.
    "spines": """
[[{:binder}], [
  ~[$x, [[{$}], $unique, {x}]],
  ~[$x.2, [[{$}], $unique, {x}]],
  ~[$cases, [~[[{:all}], $x]]],
  ~[$f, ~[[{:lambda}], ~[~, ~[[[$x]], $cases]]]],
  ~[$v1, [[{$}], $f, {a}]],
  ~[$cases2, [~[[{:all}, {:all}], ~[$x, $x.2]]]],
  ~[$f2, ~[[{:lambda}], ~[~, ~[[[$x], [$x.2]], $cases2]]]],
  ~[$g2, [[{$}], $f2, {1}]],
  ~[$v2, [[{$}], $g2, {2}]],
  ~[$y, [[{$}], $unique, {y}]],
  ~[$cl, [~[$y, {w}]]],
  ~[$box, ~[{box}, [[{:binder}], $cl, $y]]],
  ~[$unbox, [[{:lambda}], ~, [[$h, $t]], ~[[{:fork}], $t]]],
  ~[$b, [[{$}], $unbox, $box]],
  ~[$v3, $b]
 ], [$v1, $v2, $v3]]
""",
    # Recursion without a recursive binder. `fix` takes a lambda whose first parameter is the
    # recursive reference and returns the recursive function. Self-application rebuilds that
    # reference as a fresh partial application on each call, so no binding refers to itself.
    # `len` uses it with one more parameter; `app` with two more.
    "fix": """
[[{:binder}], [
  ~[$fix, [[{:lambda}], ~, [[$f]],
     ~[[{:all}],
       [[{:binder}], [
          ~[$w, [[{:lambda}], ~, [[$x], [$v]],
                  ~[[{:all}, {:all}],
                    [[{:binder}], [
                       ~[$self, [[{$}], $x, $x]],
                       ~[$fs, [[{$}], $f, $self]],
                       ~[$r, [[{$}], $fs, $v]]
                    ], $r]]]],
          ~[$g, [[{$}], $w, $w]]
       ], $g]]]],
  ~[$len, [[{:lambda}], ~, [[$rec], [$h, $t]],
     ~[[{:all}, {:leaf}], ~],
     ~[[{:all}, {:fork}], [[{:binder}], [~[$n, [[{$}], $rec, $t]]], ~[{s}, $n]]],
     ~[[{:all}, {:fruit}], {not-a-list}]]],
  ~[$length, [[{$}], $fix, $len]],
  ~[$v1, [[{$}], $length, [{a}, {b}, {c}]]],
  ~[$app, [[{:lambda}], ~, [[$rec.2], [$h.2, $t.2], [$ys]],
     ~[[{:all}, {:leaf}, {:all}], $ys],
     ~[[{:all}, {:fork}, {:all}],
       [[{:binder}], [
          ~[$part, [[{$}], $rec.2, $t.2]],
          ~[$rest, [[{$}], $part, $ys]]
       ], ~[$h.2, $rest]]]]],
  ~[$append, [[{$}], $fix, $app]],
  ~[$a1, [[{$}], $append, [{a}, {b}]]],
  ~[$v2, [[{$}], $a1, [{c}, {d}]]]
 ], [$v1, $v2]]
""",
    # Factorial and Fibonacci with `fix`, over unary numbers: `~` is zero and `~[{s}, n]` is the
    # successor of `n`, so a number is a list of `{s}`. Addition is append; multiplication and
    # both functions are made recursive with `fix`. `<F>` and `<N>` are the inputs to factorial and
    # Fibonacci.
    "arith": """
[[{:binder}], [
  ~[$fix, [[{:lambda}], ~, [[$f]],
     ~[[{:all}],
       [[{:binder}], [
          ~[$w, [[{:lambda}], ~, [[$x], [$v]],
                  ~[[{:all}, {:all}],
                    [[{:binder}], [
                       ~[$self, [[{$}], $x, $x]],
                       ~[$fs, [[{$}], $f, $self]],
                       ~[$r, [[{$}], $fs, $v]]
                    ], $r]]]],
          ~[$g, [[{$}], $w, $w]]
       ], $g]]]],
  ~[$addf, [[{:lambda}], ~, [[$rec.1], [$h.1, $t.1], [$ys]],
     ~[[{:all}, {:leaf}, {:all}], $ys],
     ~[[{:all}, {:fork}, {:all}],
       [[{:binder}], [
          ~[$p0, [[{$}], $rec.1, $t.1]],
          ~[$rest, [[{$}], $p0, $ys]]
       ], ~[$h.1, $rest]]]]],
  ~[$add, [[{$}], $fix, $addf]],
  ~[$mulf, [[{:lambda}], ~, [[$rec.2], [$h.2, $t.2], [$m]],
     ~[[{:all}, {:leaf}, {:all}], ~],
     ~[[{:all}, {:fork}, {:all}],
       [[{:binder}], [
          ~[$p1, [[{$}], $rec.2, $t.2]],
          ~[$p2, [[{$}], $p1, $m]],
          ~[$a1, [[{$}], $add, $m]],
          ~[$r1, [[{$}], $a1, $p2]]
       ], $r1]]]],
  ~[$mul, [[{$}], $fix, $mulf]],
  ~[$factf, [[{:lambda}], ~, [[$rec.3], [$h.3, $t.3]],
     ~[[{:all}, {:leaf}], [{s}]],
     ~[[{:all}, {:fork}],
       [[{:binder}], [
          ~[$p3, [[{$}], $rec.3, $t.3]],
          ~[$m1, [[{$}], $mul, ~[$h.3, $t.3]]],
          ~[$r2, [[{$}], $m1, $p3]]
       ], $r2]]]],
  ~[$fact, [[{$}], $fix, $factf]],
  ~[$fibf, [[{:lambda}], ~, [[$rec.4], [$h.4, $t.4]],
     ~[[{:all}, {:leaf}], ~],
     ~[[{:all}, {:fork}],
       [[{:binder}], [
          ~[$r3, [[{$}],
                  [[{:lambda}], ~, [[$h.5, $t.5]],
                    ~[[{:leaf}], [{s}]],
                    ~[[{:fork}],
                      [[{:binder}], [
                         ~[$f1, [[{$}], $rec.4, $t.4]],
                         ~[$f2, [[{$}], $rec.4, $t.5]],
                         ~[$a2, [[{$}], $add, $f1]],
                         ~[$r4, [[{$}], $a2, $f2]]
                      ], $r4]]],
                  $t.4]]
       ], $r3]]]],
  ~[$fib, [[{$}], $fix, $fibf]],
  ~[$out1, [[{$}], $fact, <F>]],
  ~[$out2, [[{$}], $fib, <N>]]
 ], [$out1, $out2]]
""",
    # Errors.
    "no-match": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$h, $t]], ~[[{:fork}], {f}]]],
  ~[$v, [[{$}], $f, {z}]]
 ], $v]
""",
    "fork-on-one-name": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$x]], ~[[{:all}], {first}], ~[[{:fork}], {f}]]],
  ~[$v, [[{$}], $f, ~[{1}, {2}]]]
 ], $v]
""",
    "short-row": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$x], [$y]], ~[[{:all}], {f}]]],
  ~[$g, [[{$}], $f, {1}]],
  ~[$v, [[{$}], $g, {2}]]
 ], $v]
""",
    "no-cases": """
[[{:binder}], [
  ~[$f, [[{:lambda}], ~, [[$x]]]],
  ~[$v, [[{$}], $f, {1}]]
 ], $v]
""",
}


def numeral(n):
    """The unary numeral for n: `~` or a list of n `{s}` fruits."""
    return "[" + ", ".join(["{s}"] * n) + "]" if n else "~"


if __name__ == "__main__":
    out = Path(__file__).with_name("programs")
    out.mkdir(exist_ok=True)
    for name, template in PROGRAMS.items():
        text = expand(template).strip()
        if name == "arith":
            text = text.replace("<F>", numeral(5)).replace("<N>", numeral(5))
        (out / f"{name}.v0").write_text(text + "\n")
