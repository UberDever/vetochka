"""Run the catalogue on the current scheme (cas) and on base, with the scheme's intended results.
Throwaway. Writes cas-results.md."""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import catalogue as C  # noqa: E402
import variants as V  # noqa: E402
from cas import CasMachine, SaturatingCasMachine, SimultaneousCasMachine  # noqa: E402
from programs import expand  # noqa: E402

# Under the scheme a written declaration slot declares itself, so computed declarations use the
# data form; unset names used in a body are an error; an extracted identifier fed back stays free.
DATA_FORM = {"C2", "C3", "C4", "C6", "D1", "D2", "H2"}
OVERRIDE = {
    "F1": C.error("not bound"),
    "F2": C.error("not bound"),
}


def run(machine, check, text):
    try:
        result, _, _ = machine.run(V.parse(expand(text)), V.globals_for())
    except V.MachineError as error:
        fragment = getattr(check, "error", None)
        return (fragment is not None and fragment in str(error)), "error: " + str(error)
    except RecursionError:
        return False, "host recursion overflow"
    if getattr(check, "error", None) is not None:
        return False, V.display(result)
    return check(machine, result, V.display)


def main():
    lines = ["| case | form | intended | base | cas, sequential | cas, simultaneous binders | cas |", "|---|---|---|---|---|---|---|"]
    failures = []
    for key, title, check, fs in C.entries():
        form = "data" if key in DATA_FORM else "plain"
        check = OVERRIDE.get(key, check)
        text = fs[form]
        cells = []
        for machine in (V.Machine(V.VARIANTS[0]), CasMachine(), SimultaneousCasMachine(), SaturatingCasMachine()):
            ok, shown = run(machine, check, text)
            shown = (shown if len(shown) <= 50 else shown[:47] + "...").replace("|", "\\|")
            cells.append(shown if ok is None else ("✓" if ok else "✗ " + shown))
            if type(machine) is SaturatingCasMachine and ok is False:
                failures.append((key, shown))
        lines.append(f"| {key} {title} | {form} | {check.intended} | " + " | ".join(cells) + " |")
    out = "\n".join(lines) + "\n\ncas failures: " + (", ".join(f"{k} ({s})" for k, s in failures) or "none") + "\n"
    (HERE / "cas-results.md").write_text(out)
    print(out)


if __name__ == "__main__":
    main()
