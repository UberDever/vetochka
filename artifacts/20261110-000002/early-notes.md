# Early compaction integration notes

The draft contains 107 entries on 29 dates, from 25 December 2024 through 10 July 2026. Its primary source is the complete retained history at `artifacts/20261020-000000/20260201-183604--project_history.md`. Source headings with four-digit years were normalized to the required two-digit format. No relation IDs were assigned.

The draft preserves original proposals even where later developments replaced them: fixed nodes before variable encoding, separate VM bytecode before intensional opcodes, callable native values before dedicated `call`, right-associative application before left association, Wisp before the C-oriented surface, dynamic scope before lexical scope, positional application before explicit APPLY, and reducer-first execution before machine-owned dispatch. The July 10 parser correction remains separate from the July 9 intended seam. The proposed June machine remains separate from settled Layers 0–4.

The original history mixes contemporary notes with later checkbox/discard annotations. For WAL prefix fidelity, the draft records the work when first proposed and does not invent completion/rejection dates. These undated later states need integration with other dated sources if available:

- Bytecode dumping is retrospectively marked complete in the February 15 section.
- The EBNF grammar work is retrospectively marked complete in the April 2 section.
- Byte-encoding review is retrospectively marked complete in the June 14 section.
- The C+Lua migration suggestion in the June 5 section is later marked discarded, referring to task `20260606-141938`.
- Several smaller syntax and reducer investigations carry later discard marks. The draft preserves their substantive local conclusions where the text gives them, but does not assign the unknown administrative rejection date.

March opcode research has a directory timestamp but no reliable exact internal date. The March 18 retained-history entry already carries the essential magic-position proposal, so the draft does not invent a March 22 event. The supplementary file's assertions that standalone B is impossible and that `K y x` establishes sequencing are unverified LLM claims. They are not recorded as established calculus knowledge. The file is `artifacts/20260322-120000/llm_research_on_opcode_shape.md`.

The June 15 history explicitly labels CESK material unapproved. This takes precedence over `v0_basis_review.md` calling itself a decision summary. Its lambda/CESK/ZINC preference remains useful candidate knowledge and appears as a proposal.

The July 9 history mentions a snapshot under a July 11 artifact directory. The draft preserves the stated July 9 restoration date and identifies that provenance. It does not infer that every later statement in that snapshot was already known on July 9.

`minimal-machine.md` says its dialogue started on July 10, but it may include later additions. The draft uses it only to preserve the open machine questions that fit the July 10 history. The opcode/`do!` research explicitly records a July 10 conversation and July 11 import. The draft dates its candidate content to the conversation, while excluding its later July 11 specialization correction.

The main draft intentionally omits empty June 13 cleanup bookkeeping, example inventories, repeated research introductions, and detailed byte tables. It preserves changes in node kinds and reference sizes, reflection contracts, argument policies, and native-call safety boundaries.
