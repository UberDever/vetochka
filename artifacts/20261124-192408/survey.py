#!/usr/bin/env python3
"""Find doc lines and code comments ever committed in vetochka that survive nowhere today."""
import subprocess, re, json, sys, tarfile, io, os, collections

REPO = sys.argv[1]
OUT = sys.argv[2]

def git(*a, text=True):
    return subprocess.run(["git", "-C", REPO, *a], capture_output=True, check=True, text=text, errors="replace" if text else None).stdout

DOC_RE = re.compile(r"(\.md$|\.txt$|\.tree$|TODO|README|AGENTS|JUSTIFICATION|state_schema\.json$|/tags$|\.diff$)")
CODE_RE = re.compile(r"\.(c|h|py|zig|lua|sh|ninja|yml)$|nob\.c$")
SKIP_RE = re.compile(r"^(third_party|.*vendor|.*stb_ds|.*jsmn|.*json\.h|.*arena\.h|.*da\.h)")

def norm(s):
    s = s.strip()
    s = re.sub(r"^([-*+>#]+|\d+\.|//+|/\*+|\*+|;;+|--+|#+)\s*", "", s)
    s = re.sub(r"\*/$", "", s)
    s = re.sub(r"[`*_~]", "", s)
    s = re.sub(r"\s+", " ", s).strip().lower()
    return s

def words(s):
    return re.findall(r"\w+", s)

def grams(ws, n=4):
    return {tuple(ws[i:i+n]) for i in range(len(ws)-n+1)}

COMMENT_RE = re.compile(r"(//|/\*|^\s*\*|#(?!include|define|if|endif|else|pragma|undef|elif)|--|;;)(.*)")

def comment_text(line, path):
    if path.endswith(".py") or path.endswith(".sh") or path.endswith(".yml") or path.endswith(".ninja"):
        m = re.search(r"#\s?(.*)", line)
        if m and not re.match(r"\s*['\"]", line[:m.start()][-1:] or ""):
            return m.group(1)
        m = re.match(r'\s*("""|\'\'\')?(.*)', line)
        return None
    if path.endswith(".lua"):
        m = re.search(r"--\s?(.*)", line); return m.group(1) if m else None
    m = re.search(r"(//|/\*)\s?(.*)", line)
    if m: return m.group(2)
    m = re.match(r"\s*\*\s?(.*)", line)
    if m: return m.group(1)
    return None

# ---- preserved corpus: current text in every branch tip + tarballs
corpus_lines = set()
corpus_grams = set()
def add_corpus(text):
    for l in text.splitlines():
        n = norm(l)
        if len(n) < 3: continue
        corpus_lines.add(n)
        corpus_grams.update(grams(words(n)))

tips = git("for-each-ref", "--format=%(objectname)", "refs/remotes/origin").split()
tips = list(dict.fromkeys(tips))
# only the new-lineage tips count as "preserved today" (old branches are history, not records)
new_root = git("rev-parse", "9bceff8").strip()
preserved_tips = [t for t in tips if new_root in git("rev-list", "--max-parents=0", t)]
for t in preserved_tips:
    for path in git("ls-tree", "-r", "--name-only", t).splitlines():
        if path.endswith((".pdf", ".jpg", ".png", ".gz", ".svg")): continue
        if SKIP_RE.match(path): continue
        try:
            add_corpus(git("show", f"{t}:{path}"))
        except subprocess.CalledProcessError:
            pass
for root, dirs, files in os.walk(REPO):
    dirs[:] = [d for d in dirs if d != ".git"]
    for f in files:
        p = os.path.join(root, f)
        rel = os.path.relpath(p, REPO)
        if rel.endswith((".pdf", ".jpg", ".png", ".gz", ".svg")) or SKIP_RE.match(rel):
            continue
        with open(p, encoding="utf-8", errors="replace") as fh:
            add_corpus(fh.read())
for tb in ["artifacts/20261110-000000/originals.tar.gz", "artifacts/20261110-000002/before-compaction.tar.gz"]:
    with tarfile.open(os.path.join(REPO, tb)) as tf:
        for m in tf.getmembers():
            if m.isfile() and not m.name.endswith(".pdf"):
                add_corpus(tf.extractfile(m).read().decode("utf-8", "replace"))

def covered(n):
    if n in corpus_lines: return True
    g = grams(words(n))
    if not g: return False
    return sum(1 for x in g if x in corpus_grams) / len(g) >= 0.6

# ---- walk all commits, collect added lines
commits = git("log", "--all", "--reverse", "--format=%H\t%ad\t%s", "--date=short").splitlines()
seen = {}
lost = []
for row in commits:
    h, date, subj = row.split("\t", 2)
    diff = git("show", "--format=", "--unified=0", "--no-renames", h)
    path = None
    for l in diff.splitlines():
        if l.startswith("+++ "):
            path = l[6:] if l.startswith("+++ b/") else None
            continue
        if not path or not l.startswith("+") or l.startswith("+++"): continue
        if SKIP_RE.match(path): continue
        kind = None
        text = l[1:]
        if DOC_RE.search(path) and not path.startswith(("artifacts/", "devlog/")):
            kind = "doc"
        elif CODE_RE.search(path):
            c = comment_text(text, path)
            if c is None: continue
            text = c; kind = "comment"
        else:
            continue
        n = norm(text)
        if len(words(n)) < 4: continue
        if n in seen: continue
        seen[n] = True
        if covered(n): continue
        lost.append({"commit": h[:7], "date": date, "subject": subj, "path": path, "kind": kind, "text": text.rstrip()})

json.dump(lost, open(OUT, "w"), ensure_ascii=False, indent=1)
c = collections.Counter((x["kind"], x["path"]) for x in lost)
print("lost lines:", len(lost))
for (k, p), n in sorted(c.items(), key=lambda x: -x[1])[:80]:
    print(f"{n:5d} {k:8s} {p}")
