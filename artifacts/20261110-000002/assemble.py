"""Assemble the reviewed, handwritten semantic entries for this one-time cleanup."""
from pathlib import Path
import re
from datetime import datetime

HERE = Path(__file__).resolve().parent
rows = []
for name in ('early.md', 'middle.md', 'late.md'):
    day = ''
    for line in (HERE / name).read_text(encoding='utf-8').splitlines():
        if line.startswith('### '):
            day = line[4:]
        elif line.startswith('- '):
            rows.append({'day': day, 'line': line, 'origin': name})

aliases = {
    'representation': 'encoding', 'scope': 'binders', 'module': 'binders',
    'tagging': 'encoding', 'v0': 'machine', 'evaluation': 'machine',
    'vm': 'machine', 'intensionality': 'encoding', 'function': 'binders',
    'reflection': 'encoding', 'memory': 'encoding', 'literal': 'syntax',
    'parser': 'syntax', 'calculus': 'machine', 'abi': 'encoding',
    'grammar': 'syntax', 'purpose': 'framing', 'resolution': 'binders',
    'compilation': 'machine', 'implementation': 'project', 'bootstrap': 'host',
    'recursion': 'binders', 'sequencing': 'machine', 'tailcall': 'machine',
    'layers': 'machine', 'source-porosity': 'framing',
}
for row in rows:
    m = re.match(r'^(- (?:\[[ x]\] )?)\(([^)]+)\) (.*)$', row['line'])
    assert m, row
    tags = list(dict.fromkeys('#' + aliases.get(t[1:], t[1:]) for t in m[2].split()))
    row['line'] = m[1] + '(' + ' '.join(tags) + ') ' + m[3]


def find(day: str, text: str) -> dict:
    found = [r for r in rows if r['day'] == day and text in r['line']]
    assert len(found) == 1, (day, text, len(found))
    return found[0]


find('09.02.26', 'Use a free list')['line'] = '- (#encoding #decision) Select a free list for cell reclamation. The retained note marks this choice complete; no separate implementation result is recorded.'
find('15.02.26', 'Implement the textual bytecode parser')['line'] = '- [ ] (#syntax) Implement the textual bytecode parser in the interpreter. Canonical bytecode dumping is marked complete in the retained note; its actual completion time is not independently recorded.'
find('02.04.26', 'Write a proper grammar')['line'] = '- (#syntax #evidence #question) The retained note marks an EBNF grammar document complete, without an independent completion timestamp. Reconsider whether a strict Lisp grammar fits the intended language.'
find('14.06.26', 'Revisit byte encoding')['line'] = '- (#encoding #evidence) Byte encoding is marked reviewed. No outcome is recorded, so do not infer a new layout from the completion mark.'
find('09.07.26', 'During restoration, establish Layers')['line'] = '- (#machine #decision) Separate pure triage calculus, cell storage, inert syntax, specialization, and backbone operations. Each layer adds vocabulary without making lower layers depend on higher ones. This restored account defines target semantics; exact machine state, forcing, and function handling remain open. See artifacts/20260711-120000/v0_layers.md.'
find('11.06.26', 'Native opcode states accumulate')['line'] += ' Signatures are strict, without variadic parameter lists.'
find('22.02.26', 'retained basis consists')['line'] = '- (#machine #decision) In shared vf notation, pure triage uses construction `~(x) -> ~[x]` and `~[x](y) -> ~[x,y]`, constant selection `~[~,x](y) -> x`, and distribution `~[~[x],y](z) -> x(z)(y(z))`. Its matcher `~[~[w,x],y]` applied to `~`, `~[u]`, or `~[u,v]` yields `w`, `x(u)`, or `y(u)(v)` respectively. These are the calculus equations, not an effect-order policy.'
find('22.02.26', 'In shared vf notation')['line'] += ' See artifacts/20261020-000000/20260201-183604--project_history.md, 22.02.2026.'
find('11.07.26', 'Records. Keep chronological')['line'] = '- (#process #decision) Records. Keep chronological development history and research in shared project memory, with exact examples retained as supporting artifacts.'
find('03.09.26', 'Complete-picture.')['line'] = find('03.09.26', 'Complete-picture.')['line'].replace(' and picture.md', '')
find('24.09.26', 'Design-record.')['line'] = find('24.09.26', 'Design-record.')['line'].replace('Third-party PDFs and possible copyable-example/runtime exceptions remain unresolved.', 'Keep retained third-party PDFs for now. Revisit removal or links and possible copyable-example/runtime exceptions later.')
find('29.09.26', 'C-migration.')['line'] = '- (#reducer #evidence) C-migration. Replace the Zig build/tests with muh-build and Lua tests, completing 4b0ff4c. The existing C implementation is organized into public packages; Lua bindings ship as a public package. Recorded verification reports 25 Lua tests and fresh-clone builds. These results validate the migration, not the unfinished machine semantics.'
find('06.11.26', 'Producer-return.')['line'] = find('06.11.26', 'Producer-return.')['line'].replace(' The parser-return example is subsequently handled through an ordinary lambda instead.', '')
find('07.11.26', 'Value-core.')['line'] = find('07.11.26', 'Value-core.')['line'].replace('Reconcile versioned observations', 'Reconcile older pure-nyad/opaque literal forms, versioned observations')
find('10.11.26', 'Log-contract.')['line'] += ' This authorized initial cleanup rebuilds IDs. Prior import IDs belong to the saved preceding log; subsequent writes preserve the compacted IDs. Reference scope: artifacts/20261110-000002/references.json.'

# Restore contracts and pending work that must be recoverable without opening artifacts.
rows.append({'day':'05.08.26', 'line':'- (#encoding #decision) Pith-contract. A pith exposes a node\'s introspectable internals on Rule 3 demand. Constructing the same pith must represent the same node, extending intensionality beyond pure trees to opcodes and other runtime entities. The handwritten spec records this contract in Git dbb6c0f; individual family projections remain incomplete.', 'origin':'review correction'})
rows.append({'day':'24.09.26', 'line':'- (#encoding #decision) Pith-depth. Expose one layer, with piths as children; nyad-built terms are their own pith. Primitive runtime kinds must define their projections. This is a direction for the specification, not an implemented pith inventory. First recorded in Git 9bceff8. Review sober with the other representation rulings.', 'origin':'review correction'})
rows.append({'day':'05.08.26', 'line':'- [ ] (#framing #spec) Influences. Explain the influences of tree/triage calculus, lambda calculus, C, ML, and Scheme. Give each a source and identify the Vetochka level it shaped. The handwritten introduction already requests this in Git dbb6c0f; the explanations remain pending.', 'origin':'review correction'})
find('30.08.26', 'Triage-boundary.')['line'] = '- (#machine #decision) Triage-boundary. Rule 3 never executes `{@}` data. The final August ruling permits evaluating a pending machine application to expose its root shape before Rule 3 dispatch. July SHAPE-APPLY survives only at that machine boundary.'
control = rows.index(find('30.08.26', 'Control.'))
rows.insert(control, {'day':'30.08.26', 'line':'- (#machine #decision) Calculus-control. Retain rules 0..3 and branch-first evaluation rather than the agent\'s eager-operand proposal. Rule 2 copies raw operands, so duplicated effects are accepted under this calculus evaluator.', 'origin':'review correction'})
find('30.08.26', 'Control.')['line'] = '- (#machine #decision) Control. CESK control position determines execution timing. Require proper tail calls: allocate frames only when a sub-result is needed; otherwise replace control.'
rows.append({'day':'30.08.26', 'line':'- (#machine #decision) Block-frames. Select opcode/BLOCK-frame execution for do...end statement lists rather than desugaring them into bind-discard applications.', 'origin':'review correction'})
find('26.10.26', 'Blocks.')['line'] = '- (#machine #decision #question) Blocks. Executing a nonempty block forces entries in source order, discards intermediate results, and returns the last result. Constructing or returning the block as data does not execute it. The lambda redesign reopens the earlier BLOCK-frame choice: sequencing by bindings or a dedicated block operation remains unsettled. Define the empty-block result too.'
find('26.10.26', 'Lambda-core.')['line'] = '- (#machine #decision) Lambda-core. Remove primitive rule 0 as well as rules 1 and 2. Lambda application can express its construction behavior, for example `(\\[x] do ~[x] end)(~)`. Retain trees and structural triage as design principles, not the original evaluator. See artifacts/20260529-103914/2026-10-26.md.'
rows.append({'day':'26.10.26', 'line':'- (#machine #proposal) Application-only. Consider making applications the only reducible forms and lowering binding and triage to applications. Callees, encodings, and their execution contracts remain unspecified. Tagging becomes an encoding concern for execution and representation; syntax lowering alone does not establish semantics.', 'origin':'review correction'})

# Separate changes that receive different later resolutions.
r = find('30.08.26', 'Staging. Applications in data')
r['line'] = '- (#machine #decision) Application-data. Applications in data are always inert `{@}` markers. Building these lists executes constructors, not the represented application.'
rows.append({'day': '30.08.26', 'line': '- (#machine #decision) Staging. Host-side exec is the explicit entry into machine control. Reject a second handwritten interpreter and remove the large specialize stage. The machine proceeds by applications and results; names survive until lookup.', 'origin': 'integration'})
r = find('21.10.26', 'Notation.')
r['line'] = '- (#syntax #decision) Notation. Use explicit `\\[params]` lambdas, arbitrary-term annotations, and first-class blocks and labels. Shared parsing fixes grouping without operator precedence; interpreters assign operator meaning. The broad syntax rewrite is recorded in Git `39f7116`, not demonstrated as evaluator implementation.'
rows.append({'day':'21.10.26', 'line':'- (#encoding #decision) Homogeneous-core. The core admits only homogeneous leaf, stem, and fork nyads as primitive tree structure. Native literal representation and observation remain incomplete.', 'origin':'review correction'})
rows.append({'day':'21.10.26', 'line':'- (#syntax #decision) Tree-notation. Bare `~` replaces the empty-bracket leaf spelling. A stem is `~[x]` and a fork is `~[x,y]`. Proper lists are right-nested forks ending in `~`, for example `[x,y] == ~[x,~[y,~]]`. These notation conventions are separate from the choice of observable primitive kinds.', 'origin':'review correction'})
rows.append({'day':'21.10.26', 'line':'- (#syntax #decision) Lexical. Source is UTF-8. Strings use balanced `{...}` with no escapes. Comments use `;;`, nested `#| ... |#`, and datum-discarding `#;`. Decimal integers are distinct from prefix application of a minus operator. The documented lexical rules and implementation must agree.', 'origin':'integration'})
r = find('26.10.26', 'Grammar-edit.')
r['line'] = '- (#syntax #evidence) Grammar-edit. The syntax draft gains binding and three-clause triage forms, full-expression nyad children, and trailing commas. These edits do not supply execution rules or parser implementation. See artifacts/20241220-144435/2026-10-26.md.'
rows.append({'day':'26.10.26', 'line':'- (#syntax #encoding #evidence) Literal-tags. An agent edit restricts annotation headers to literal comma-separated lists and gives adjacent headers concatenating composition. This is the temporary documented shape, not a completed tagging implementation.', 'origin':'integration'})
rows.append({'day':'26.10.26', 'line':'- (#syntax #evidence) Header-layout. The syntax edit adds ASI exceptions connecting binding/triage headers to their blocks and connecting triage clauses. This is a temporary documentation rule; parser implementation remains pending.', 'origin':'integration'})
r = find('07.11.26', 'Literals.')
r['line'] = '- [ ] (#syntax #question) Literals. Choose Unicode identifier admission and extensible number syntax, including decimal/exponent floating forms. ASCII-only identifiers remain an implementation limitation. Check continuation spacing, datum comments with ASI, and opt-in source-location metadata. Formatter idempotence remains the unresolved September 29 issue.'
find('07.11.26', 'Delivery.')['line'] = find('07.11.26', 'Delivery.')['line'].replace('in the old reducer', 'in public/reducer/reducer.c')

# Keep same-day source order; inserted refinements follow their day, before later changes.
rows.sort(key=lambda r: datetime.strptime(r['day'], '%d.%m.%y'))
indices = {}
for row in rows:
    row['index'] = indices.get(row['day'], 0)
    indices[row['day']] = row['index'] + 1

relations = [
    (('30.08.26','Nyads.'), ('07.08.26','Explicit-arity.')),
    (('30.08.26','Escaping-results.'), ('18.07.26','Environment-flow.')),
    (('24.09.26','Universal-piths.'), ('21.07.26','LENS.')),
    (('25.09.26','Non-evaluating-Rule3.'), ('30.08.26','Triage-boundary.')),
    (('26.09.26','Designator-dispatch.'), ('21.07.26','State-tags.')),
    (('20.10.26','Adjacency.'), ('30.08.26','Adjacency.')),
    (('23.10.26','Applications.'), ('30.08.26','Application-data.')),
    (('23.10.26','Basis.'), ('30.08.26','Calculus-control.')),
    (('26.10.26','Lambda-core.'), ('23.10.26','Basis.')),
    (('26.10.26','Blocks.'), ('30.08.26','Block-frames.')),
    (('27.10.26','Arbitrary-tags.'), ('26.10.26','Literal-tags.')),
    (('27.10.26','Layout.'), ('26.10.26','Header-layout.')),
    (('07.11.26','Substrate.'), ('21.10.26','Homogeneous-core.')),
]
for newer, older in relations:
    current, previous = find(*newer), find(*older)
    assert rows.index(previous) < rows.index(current)
    previous['line'] = previous['line'].replace(') ', ' #superseded) ', 1)
    match = re.match(r'^(- (?:\[[ x]\] )?\([^)]*\) )([^ .]+\. )(.*)$', current['line'])
    assert match, current
    target_title = re.match(r'^- (?:\[[ x]\] )?\([^)]*\) ([^ .]+)\. ',previous['line'])
    title = f' "{target_title[1]}"' if target_title else ''
    current['line'] = f'{match[1]}{match[2]}Supersession for ({previous["day"]}, {previous["index"]}){title}. {match[3]}'

out = []
last = ''
for row in rows:
    if row['day'] != last:
        out.extend(['', '### ' + row['day']])
        last = row['day']
    out.append(row['line'])
(HERE / 'candidate.md').write_text('\n'.join(out).lstrip() + '\n', encoding='utf-8')
print(f'{len(rows)} entries across {len(indices)} dates; {len(relations)} explicit supersession relations.')
