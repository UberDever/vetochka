"""Check the compacted WAL, references, and retained source integrity."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import re
import stat
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
path = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'devlog/log.md'
entries = {}
dates = []
day = None
index = 0
allowed = {'syntax', 'encoding', 'binders', 'machine', 'reducer', 'opcode', 'host', 'framing', 'spec', 'process', 'project', 'experiment', 'log', 'optimization', 'decision', 'proposal', 'evidence', 'question', 'superseded'}
for line in path.read_text(encoding='utf-8').splitlines():
    if line.startswith('### '):
        assert re.fullmatch(r'### \d{2}\.\d{2}\.\d{2}', line), line
        day = line[4:]
        parsed = datetime.strptime(day, '%d.%m.%y').date()
        assert not dates or parsed > dates[-1], line
        dates.append(parsed)
        index = 0
    elif line.startswith('- '):
        assert day is not None
        match = re.fullmatch(r'- (?:\[[ x]\] )?\((#[^()]+)\) (\S.*)', line)
        assert match, line
        tags = match[1].split()
        assert len(tags) == len(set(tags)), line
        assert {tag[1:] for tag in tags} <= allowed, line
        entries[f'{day},{index}'] = line
        index += 1
        for ref in re.findall(r'artifacts/[\w./-]+', line):
            assert (ROOT / ref.rstrip('.')).is_file(), ref
        assert 'Source: `' not in line, 'Mechanical source locators remain'
assert dates[0].isoformat() == '2024-12-25'
assert dates[-1].isoformat() == '2026-11-10'
positions = {key: number for number, key in enumerate(entries)}
for key, line in entries.items():
    for date, number in re.findall(r'(?:Supersession for|Link to) \((\d{2}\.\d{2}\.\d{2}), (\d+)\)', line):
        target = f'{date},{number}'
        assert target in entries, (key, target)
        if f'Supersession for ({date}, {number})' in line:
            assert positions[target] < positions[key], (key, target)
            assert '#superseded' in entries[target], (key, target)
for item in json.loads((HERE / 'before.json').read_text()):
    with tarfile.open(HERE / 'before-compaction.tar.gz') as archive:
        member = archive.extractfile(item['path'])
        assert member is not None
        assert hashlib.sha256(member.read()).hexdigest() == item['sha256']
original = ROOT / 'artifacts/20261110-000000'
manifest = json.loads((original / 'manifest.json').read_text())
missing_generated = []
with tarfile.open(original / 'originals.tar.gz') as archive:
    for source in manifest['files']:
        member = archive.extractfile(source['source'])
        assert member is not None
        assert hashlib.sha256(member.read()).hexdigest() == source['source_sha256']
        target = ROOT / source['artifact']
        generated = source['source'] in {'archive/new_spec/output.pdf', 'archive/spec/output.pdf'} or ('/__pycache__/' in source['source'] and source['source'].endswith('.pyc'))
        if not target.exists() and generated:
            missing_generated.append(source['artifact'])
            continue
        assert hashlib.sha256(target.read_bytes()).hexdigest() == source['artifact_sha256']
        # Git preserves executable status; the archive retains the full original permissions.
        assert bool(target.stat().st_mode & stat.S_IXUSR) == bool(archive.getmember(source['source']).mode & stat.S_IXUSR)
if len(sys.argv) == 1:
    assert not list((ROOT / 'devlog').glob('log.md.*')), 'Redundant imported rotations remain active'
    for name in ('README.md', 'public/reducer/reducer.c', 'tests/source_test.lua'):
        for date, number in re.findall(r'\((\d{2}\.\d{2}\.\d{2}), (\d+)\)', (ROOT / name).read_text()):
            assert f'{date},{number}' in entries, (name, date, number)
print(f'Verified {len(entries)} entries across {len(dates)} dates, relations, citations, prior-log backup, {len(manifest["files"])} archived originals, and {len(manifest["files"]) - len(missing_generated)} retained artifacts.')
if missing_generated:
    print('Generated outputs absent from artifact folders, preserved in original backup: ' + ', '.join(missing_generated))
