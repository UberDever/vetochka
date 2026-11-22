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
    # application must still see its own declarations, not the argument (22.11.26, 29).
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


if __name__ == "__main__":
    out = Path(__file__).with_name("programs")
    out.mkdir(exist_ok=True)
    for name, template in PROGRAMS.items():
        (out / f"{name}.v0").write_text(expand(template).strip() + "\n")
