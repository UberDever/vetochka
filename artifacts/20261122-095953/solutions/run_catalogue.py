"""Run the catalogue on every variant and write results.md. Throwaway."""

import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import catalogue  # noqa: E402
import variants as V  # noqa: E402
from programs import expand  # noqa: E402


def prepare(machine, text):
    tree = V.parse(expand(text))
    if machine.c.roles:
        tree = V.to_roles(machine, tree)
    return tree


def run_one(config, check, text):
    machine = V.Machine(config)
    try:
        tree = prepare(machine, text)
        result, _, steps = machine.run(tree, V.globals_for())
    except V.MachineError as error:
        message = str(error)
        fragment = getattr(check, "error", None)
        if fragment is not None:
            return (fragment in message), "error: " + message
        return False, "error: " + message
    except RecursionError:
        return False, "host recursion overflow"
    if getattr(check, "error", None) is not None:
        return False, V.display(result)
    ok, shown = check(machine, result, V.display)
    return ok, shown


def persistence(config):
    """I1: persist `g(g(id))` as plain text, reload it in a new run, apply it to {7}."""
    machine = V.Machine(config)
    try:
        h, _, _ = machine.run(prepare(machine, catalogue.COMPOSE_H), V.globals_for())
        reloaded = V.parse(V.display(h))
        o = V.make_identifier(b"o", b"\x7f")
        if config.roles:
            program = V.lst([V.lst([b":binder"]),
                             V.lst([V.Fork(V.Fork(b":decl", o), V.lst([V.lst([b"$"]), reloaded, b"7"]))]),
                             V.Fork(b":use", o)])
        else:
            program = V.lst([V.lst([b":binder"]), V.lst([V.Fork(o, V.lst([V.lst([b"$"]), reloaded, b"7"]))]), o])
        result, _, _ = V.Machine(config).run(program, V.globals_for())
    except V.MachineError as error:
        return False, "error: " + str(error)
    shown = V.display(result)
    return shown == "{7}", shown


def main():
    rows = []
    started = time.time()
    for key, title, check, fs in catalogue.entries():
        for form_name, text in fs.items():
            cells = [run_one(config, check, text) for config in V.VARIANTS]
            rows.append((key, title, form_name, check.intended, cells))
    rows.append(("I1", "persist g(g(id)) as plain text, reload in a new run, apply", "plain", "{7}",
                 [persistence(config) for config in V.VARIANTS]))
    elapsed = time.time() - started

    def cell(ok, shown):
        shown = shown if len(shown) <= 60 else shown[:57] + "..."
        shown = shown.replace("|", "\\|")
        if ok is None:
            return shown
        return ("✓" if ok else "✗ " + shown)

    lines = ["| case | form | intended | " + " | ".join(c.name for c in V.VARIANTS) + " |",
             "|---|---|---|" + "---|" * len(V.VARIANTS)]
    for key, title, form_name, intended, cells in rows:
        lines.append(f"| {key} {title} | {form_name} | {intended} | "
                     + " | ".join(cell(ok, shown) for ok, shown in cells) + " |")
    totals = []
    for index, config in enumerate(V.VARIANTS):
        checked = [r[4][index][0] for r in rows if r[4][index][0] is not None]
        totals.append(f"{config.name}: {sum(1 for x in checked if x)}/{len(checked)}")
    out = "\n".join(lines) + "\n\nPassed per variant (all forms): " + ", ".join(totals) + "\n"
    (HERE / "results.md").write_text(out)
    print(out)
    print(f"({elapsed:.1f} s)")


if __name__ == "__main__":
    main()
