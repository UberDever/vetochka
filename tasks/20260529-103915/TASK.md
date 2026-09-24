# Framing: reflective C frontend and its appliances

- STATUS: OPEN
- PRIORITY: 100
- TAGS: epic, framing

- (user, 2026-05-28) A reflective, staged, C-emitting language. No language semantics leak into the generated C.
- (user) Porosity is the motivation: stay porous to other systems like C, and reflectively porous inside. See porosity.md.
- (user, 2026-08-30) Stack: v0 machine, then vmeta, then a vsystem compiler written in vmeta that emits ordinary C.
- (user, 2026-08-30) Internal porosity exists at build time only; the emitted C carries external porosity. vsystem has no runtime reflection.
- (user, 2026-08-30) An appliance is a language plus mandated passes. Disciplines like ownership become analysis libraries.
- (user, 2026-08-30) The substrate is permissive; appliances restrict at their own compile step.

## Tasks

- [ ] 20260805-162237 Explain each inspiration source in the intro
