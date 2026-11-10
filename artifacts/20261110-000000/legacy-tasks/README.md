# Tasks

The following tasks hold the current design and implementation work:

| Subject | Record |
| --- | --- |
| Syntax and lowering | 20241220-144435 |
| Floating-point literal syntax | 20261020-103514 |
| Values, observation, and reconstruction | 20260926-091318 |
| Bindings, functions, and mutation | 20241225-140425 |
| Evaluation and the v0 machine | 20260529-103914 |
| Native operations and host integration | 20250123-091611 |

Start with the value review. The machine's `items.md` retains the delivery order.
Each subject has one owner. The floating-point issue owns its grammar decision and references the broader implementation tasks.
Supporting records are five enduring intents, not five more implementation tasks.
Read `KIND` and `PARENT` to recover the hierarchy.

The October 20 rewrite replaced the previous 72-record set.
Original records and artifacts are in `archive/20261020-000000/`.
There are no closed redirect records. Historical IDs in the archive identify historical records.
Search and index `tasks/` and `docs/` only.

List current records with `/home/uberdever/dev/agents/pi-box/box tatr ls` from the project root.
The format is tatr's. Project-specific `KIND` and `PARENT` properties define the hierarchy.
Tool source: https://github.com/tsoding/tatr
