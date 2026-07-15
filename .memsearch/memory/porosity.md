## Motivation: Porosity

Vetochka explores **porosity** as a language property.

C is valuable not merely because it is small or fast, but because it is porous: other systems can attach to it at many levels — source text, headers, ABI, object files, memory layout, tooling, analysis, proofs, and foreign runtimes. Its boundaries are thin enough to make it a substrate.

Reflective calculi suggest a different kind of porosity. Instead of being porous mainly to other systems, they are porous to themselves: code, data, syntax, values, terms, evaluators, and transformers can inhabit one shared representational space.

This project is motivated by the question:

> Can a practical language/runtime preserve C-like external porosity while also providing reflective internal porosity?

The goal is not maximal dynamism or “better C”. The goal is controlled permeability between levels that are usually sealed: program and data, object-level and meta-level, pure term and machine action, representation and execution.

Vetochka treats triage-calculus as a minimal semantic nucleus for this investigation: a small uniform term world where programs can inspect, construct, and transform program structure, while practical execution may still be delegated to explicit machine-level evaluators.
