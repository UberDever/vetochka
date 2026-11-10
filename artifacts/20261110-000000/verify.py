"""Independent acceptance check for the migrated corpus."""
import hashlib
import json
import re
import tarfile
import sys
import stat
from datetime import datetime
from pathlib import Path

origin = Path(__file__).resolve().parents[2]
root = Path(sys.argv[1]) if len(sys.argv) > 1 else origin
audit = root / 'artifacts/20261110-000000'
assert (root / 'devlog/log.md').exists(), 'migration has not produced a log'
manifest = json.loads((audit / 'manifest.json').read_text())
with tarfile.open(origin / 'artifacts/20261110-000000/originals.tar.gz') as archive:
    for source in manifest['files']:
        member = archive.extractfile(source['source'])
        assert member is not None
        assert hashlib.sha256(member.read()).hexdigest() == source['source_sha256']
        target = root / source['artifact']
        assert target.is_file(), source
        assert hashlib.sha256(target.read_bytes()).hexdigest() == source['artifact_sha256']
        assert stat.S_IMODE(target.stat().st_mode) == archive.getmember(source['source']).mode, f"File mode changed: {target}"
        assert source['entry_ids'], source
        assert not (root / source['source']).exists(), source

entries = {}
dates = []
rotations = sorted((root / 'devlog').glob('log.md.*'), key=lambda p: int(p.name.split('.')[2]), reverse=True)
paths = rotations + [root / 'devlog/log.md']
for path in paths:
    section = None
    index = 0
    for line in path.read_text().splitlines():
        if line.startswith('### '):
            section = datetime.strptime(line[4:], '%d.%m.%y').date()
            assert not dates or section > dates[-1], (dates[-1], section)
            dates.append(section)
            index = 0
        elif line.startswith('- '):
            assert section is not None
            assert re.match(r'^- (?:\[[ x]\] )?\(#[^()]+\) \S', line), line
            date = section.strftime('%d.%m.%y')
            entries[f'{date},{index}'] = line
            index += 1
for source in manifest['files']:
    for entry_id in source['entry_ids']:
        assert entry_id in entries, entry_id
for entry_id, line in entries.items():
    for relation, target_date, target_index in re.findall(r'(Supersession for|Link to) \((\d{2}\.\d{2}\.\d{2}), (\d+)\)', line):
        target = f'{target_date},{target_index}'
        assert target in entries, (entry_id, target)
        if relation == 'Supersession for':
            assert '#superseded' in entries[target], (entry_id, target)
assert len(entries) == manifest['entry_count'], (len(entries), manifest['entry_count'])
assert len(manifest['files']) == manifest['source_count']
print(f"Verified {len(manifest['files'])} source files, {len(entries)} entries, {len(dates)} increasing date sections, hashes and relation targets.")
