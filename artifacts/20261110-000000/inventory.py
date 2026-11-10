from pathlib import Path
import hashlib, json, subprocess, tarfile

root = Path(__file__).resolve().parents[2]
audit = root / 'artifacts/20261110-000000'
files = sorted(p for d in ('archive', 'tasks') for p in (root / d).rglob('*') if p.is_file())
rows = []
for path in files:
    name = path.relative_to(root).as_posix()
    aliases = [name]
    if name.startswith('archive/new_spec/'):
        aliases.append(name.replace('archive/new_spec/', 'docs/new_spec/', 1))
    elif name.startswith('archive/spec/'):
        aliases.append(name.replace('archive/spec/', 'docs/spec/', 1))
    history = ''
    git_path = name
    committed = False
    for alias in aliases:
        evidence = subprocess.run(['git', 'log', '--follow', '--format=%H%x09%aI%x09%cI%x09%s', '--', alias], cwd=root, capture_output=True, text=True, check=True).stdout
        if evidence:
            history, git_path = evidence, alias
            blob = subprocess.run(['git', 'show', f'HEAD:{alias}'], cwd=root, capture_output=True)
            committed = blob.returncode == 0 and blob.stdout == path.read_bytes()
            break
    rows.append({'source': name, 'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'bytes': path.stat().st_size, 'git_path': git_path, 'git_history': history.splitlines(), 'matches_HEAD': committed})
assert not (audit / 'originals.tar.gz').exists(), 'Original snapshot already exists; do not overwrite it'
with tarfile.open(audit / 'originals.tar.gz', 'w:gz') as archive:
    for row in rows:
        archive.add(root / row['source'], arcname=row['source'], recursive=False)
(audit / 'inventory.json').write_text(json.dumps(rows, indent=2, ensure_ascii=False) + '\n')
print(f'Preserved all {len(rows)} source files ({sum(r["bytes"] for r in rows)} bytes), including uncommitted changes and ignored files.')
print(f'{sum(bool(r["git_history"]) for r in rows)} files have Git history; {sum(r["matches_HEAD"] for r in rows)} match their HEAD source.')
