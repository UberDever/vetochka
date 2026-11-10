"""One-time migration recipe; standard-library-only, with no installation step.

Run at the repository root. Build in /tmp first; publish only after validation.
The original byte snapshot and inventory are required and never overwritten.
"""

from __future__ import annotations

import hashlib
import json
import posixpath
import re
import shutil
import sys
import tarfile
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path


ROOT = Path.cwd()
AUDIT = Path("artifacts/20261110-000000")
STAGE = Path("/tmp/vetochka-devlog-migration")
TODAY = date(2026, 11, 10)
HUID = re.compile(r"\d{8}-\d{6}")
DATE_HEADING = re.compile(r"^#{1,6} +(\d{4}-\d{2}-\d{2}|\d{2}\.\d{2}\.\d{2}(?:\d{2})?)(?=\s|:|$)")
META = re.compile(r"^- (STATUS|PRIORITY|TAGS|KIND|PARENT): *(.*)$")


@dataclass
class Entry:
    day: date
    tags: list[str]
    body: str
    source: str
    start: int
    end: int
    title: str = "Note"
    state: str = ""
    basis: str = ""
    old_id: str = ""
    relation: Entry | None = None
    index: int = -1

    @property
    def identity(self) -> str:
        return f"{self.day:%d.%m.%y},{self.index}"


@dataclass
class Source:
    row: dict
    data: bytes
    target: str
    entries: list[Entry] = field(default_factory=list)


def destination(name: str) -> str:
    parts = name.split("/")
    if HUID.fullmatch(parts[1]):
        return "/".join(["artifacts", *parts[1:]])
    if parts[0] == "archive":
        return "/".join(["artifacts", "20261110-000001", *parts[1:]])
    return "/".join([str(AUDIT), "legacy-tasks", *parts[1:]])


def parse_day(text: str) -> date:
    if "-" in text:
        return date.fromisoformat(text)
    day, month, year = text.split(".")
    return date(int(year) + (2000 if len(year) == 2 else 0), int(month), int(day))


def date_basis(source: Source) -> tuple[date, str]:
    name = source.row["source"]
    named = re.findall(r"\d{4}-\d{2}-\d{2}", Path(name).name)
    if named:
        return date.fromisoformat(named[-1]), "explicit filename date"
    if Path(name).suffix == ".md":
        for number, line in enumerate(source.data.decode("utf-8-sig").splitlines()[:8], 1):
            heading = DATE_HEADING.match(line)
            if heading:
                return parse_day(heading[1]), f"explicit document date at line {number}"
            gathered = re.match(r"^(?:Gathered|Date:|Dated:) +(\d{4}-\d{2}-\d{2})\b", line)
            if gathered:
                return date.fromisoformat(gathered[1]), f"explicit document collection date at line {number}"
    if source.row["matches_HEAD"] and source.row["git_history"]:
        commit, authored, committed, _ = source.row["git_history"][0].split("\t", 3)
        return date.fromisoformat(committed[:10]), f"Git snapshot {commit}; not a decision date"
    return TODAY, "migration snapshot; content date unresolved"


def metadata(lines: list[str]) -> dict[str, str]:
    return {match[1]: match[2] for line in lines if (match := META.fullmatch(line))}


def kinds(text: str, context: str) -> list[str]:
    lead = re.sub(r"^[~* _]+", "", text).lower()
    tags = []
    if re.match(r"(?:decision|ruling)\b|accepted(?: \([^)]*\))?:", lead) or re.fullmatch(r"decisions?|rulings?", context.split(" / ")[-1].lower()):
        tags.append("decision")
    if re.match(r"(?:proposal|proposed|candidate|hypothesis|suggestion)\b", lead) or re.fullmatch(r"proposals?|hypothes[ie]s", context.split(" / ")[-1].lower()):
        tags.append("proposal")
    if re.match(r"(?:evidence|finding|recorded result|result|observation|recorded evidence)\b", lead) or re.fullmatch(r"evidence|findings?|results?", context.split(" / ")[-1].lower()):
        tags.append("evidence")
    if re.match(r"(?:open|question|unresolved|open question)\b", lead) or re.search(r"\b(?:open questions|open details|open cruxes|unresolved questions)\b", context.lower()):
        tags.append("question")
    if re.match(r"experiment\b", lead) or "experiment" in context.lower():
        tags.append("experiment")
    return tags


def unique(values: list[str]) -> list[str]:
    return list(dict.fromkeys(values))


def main() -> None:
    rows = json.loads((ROOT / AUDIT / "inventory.json").read_text())
    sources = {}
    with tarfile.open(ROOT / AUDIT / "originals.tar.gz") as archive:
        for row in rows:
            member = archive.extractfile(row["source"])
            assert member is not None
            data = member.read()
            assert hashlib.sha256(data).hexdigest() == row["source_sha256"]
            row["source_mode"] = archive.getmember(row["source"]).mode
            source = Source(row, data, destination(row["source"]))
            sources[row["source"]] = source
    assert len({source.target for source in sources.values()}) == len(sources), "artifact collision"

    # Resolve original task-relative links in the flattened October archive too.
    aliases = {name: source.target for name, source in sources.items()}
    for name, source in sources.items():
        if name.startswith("archive/new_spec/") or name.startswith("archive/spec/"):
            aliases[name.replace("archive/", "docs/", 1)] = source.target
        if name.startswith("archive/20261020-000000/"):
            leaf = Path(name).name
            if "--" in leaf and HUID.fullmatch(leaf.split("--", 1)[0]):
                old, filename = leaf.split("--", 1)
                aliases.setdefault(f"tasks/{old}/{filename}", source.target)
    for name, source in sources.items():
        if name.startswith("tasks/"):
            aliases[name] = source.target
    aliases["archive/20260613-120000/project/v0_cesk_draft.md"] = aliases["archive/20260613-120000/v0_cesk_draft.md"]
    aliases["archive/20260613-120000/project/v0_basis_review.md"] = aliases["archive/20260613-120000/v0_basis_review.md"]
    aliases["reducer/cells_api.h"] = "https://github.com/UberDever/vetochka/blob/b9ef3ff/reducer/cells_api.h"

    subject_by_id = {}
    for name, source in sources.items():
        if name.endswith("/TASK.md"):
            meta = metadata(source.data.decode("utf-8-sig").splitlines())
            subject_by_id[name.split("/")[1]] = re.findall(r"[\w/.-]+", meta.get("TAGS", "process"))
    archived_source = sources["archive/20261020-000000/records.md"]
    archived_text = archived_source.data.decode("utf-8-sig")
    for match in re.finditer(r"<!-- BEGIN tasks/(\d{8}-\d{6})/TASK.md -->\n(.*?)<!-- END tasks/\1/TASK.md -->", archived_text, re.S):
        subject_by_id.setdefault(match[1], re.findall(r"[\w/.-]+", metadata(match[2].splitlines()).get("TAGS", "process")))

    entries = []
    current_records = {}
    previous_records = {}

    def add(source: Source, day: date, tags: list[str], body: str, start: int, end: int, **kwargs: str) -> Entry:
        tags = [tag for tag in unique(tags) if tag not in {"rejected", "historical", "snapshot", "rationale", "constraint"}]
        entry = Entry(day, tags or ["process"], body, source.row["source"], start, end, **kwargs)
        entries.append(entry)
        source.entries.append(entry)
        return entry

    def extract(source: Source, text: str, line_offset: int = 0, record_id: str = "", archived_record: bool = False) -> None:
        lines = text.splitlines()
        day, basis = date_basis(source)
        if archived_record:
            day, basis = date(2026, 10, 20), "archived task snapshot from October 20 consolidation; not the task ID date"
        headings = [line.lstrip("# ") for line in lines if re.match(r"^#{1,6} ", line)]
        title = headings[0] if headings else Path(source.row["source"]).name
        meta = metadata(lines)
        source_name = source.row["source"]
        old_id = record_id or (source_name.split("/")[1] if source_name.endswith("/TASK.md") else "")
        subject = subject_by_id.get(old_id or source_name.split("/")[1], [])
        if not subject and "--" in Path(source_name).name:
            subject = subject_by_id.get(Path(source_name).name.split("--", 1)[0], [])
        if not subject:
            group = source_name.split("/")[1]
            subject = {
                "20260322-120000": ["opcode", "proposal"],
                "20260613-120000": ["machine"],
                "20260711-120000": ["machine"],
                "20260717-120000": ["binders"],
                "20260807-120000": ["syntax"],
                "20260903-120000": ["spec", "experiment", "proposal"],
                "20260924-120000": ["process"],
                "new_spec": ["spec"], "spec": ["spec"],
            }.get(group, ["process"])
        subject = list(subject)
        if meta.get("PRIORITY", "").isdigit() and int(meta["PRIORITY"]) > 0:
            subject.insert(0, str(min(99, int(meta["PRIORITY"]))))
        is_archive = source_name.startswith("archive/")
        state = ""
        if old_id and not archived_record:
            state = "x" if meta.get("STATUS") == "CLOSED" else " "
        provenance = "Archived task snapshot" if archived_record else "Record snapshot" if old_id else "Document"
        desc = f"{provenance}: {title}."
        if old_id:
            desc += f" Legacy ID {old_id}; recorded status {meta.get('STATUS', 'unspecified')}; kind {meta.get('KIND', 'unspecified')}."
        if meta.get("PARENT"):
            desc += f" Parent: {meta['PARENT']}."
        if "snapshot" in basis:
            desc += f" Date basis: {basis}."
        header = add(source, day, subject, desc, line_offset + 1, line_offset + max(1, len(lines)), title="Record" if old_id else "Document", state=state, basis=basis, old_id=old_id)
        if old_id:
            (previous_records if archived_record else current_records)[old_id] = header
        context = []
        default_day, default_basis = day, basis
        date_sections = []
        position = 0
        while position < len(lines):
            line = lines[position]
            start = position
            if not line.strip() or META.fullmatch(line) or re.fullmatch(r"\s*(?:---+|\*\*\*+)\s*", line):
                position += 1
                continue
            if line.startswith("<!--"):
                while position < len(lines) and "-->" not in lines[position]:
                    position += 1
                position += 1
                continue
            heading = re.match(r"^(#{1,6}) +(.*)", line)
            if heading:
                level = len(heading[1])
                date_sections = [item for item in date_sections if item[0] < level]
                day, basis = date_sections[-1][1:] if date_sections else (default_day, default_basis)
                context = [item for item in context if item[0] < level]
                dated = DATE_HEADING.match(line)
                if dated:
                    day = parse_day(dated[1])
                    basis = f"explicit document date at line {line_offset + position + 1}"
                    date_sections.append((level, day, basis))
                    qualifier = line[dated.end():].lstrip(" :-").strip()
                    if qualifier:
                        context.append((level, qualifier))
                else:
                    context.append((level, heading[2]))
                position += 1
                continue
            scope = " / ".join(item[1] for item in context)
            fence = re.match(r"^\s*(`{3,}|~{3,})", line)
            if fence:
                marker = fence[1]
                position += 1
                while position < len(lines):
                    closing = lines[position].strip()
                    position += 1
                    if closing.startswith(marker[0] * len(marker)):
                        break
                body = f"Code excerpt: {scope or title}."
                add(source, day, subject, body, line_offset + start + 1, line_offset + position, title="Code", basis=basis)
                continue
            if re.match(r"^\s*\|", line):
                position += 1
                while position < len(lines) and lines[position].lstrip().startswith("|"):
                    position += 1
                add(source, day, subject, f"Table: {scope or title}.", line_offset + start + 1, line_offset + position, title="Table", basis=basis)
                continue
            # A Markdown list item and its indented continuation form one record.
            bullet = re.match(r"^([-*+] |\d+[.)] )(.*)", line)
            position += 1
            while position < len(lines):
                nxt = lines[position]
                if not nxt.strip() or re.match(r"^#{1,6} |^([-*+] |\d+[.)] )|^\s*(`{3,}|~{3,})|^\s*\|", nxt):
                    break
                position += 1
            raw = " ".join(part.strip() for part in lines[start:position]).strip()
            if bullet:
                raw = re.sub(r"^([-*+] |\d+[.)] )", "", raw)
            item_state = ""
            checkbox = re.match(r"^\[([ xX])\] +(.*)", raw)
            if checkbox:
                raw = checkbox[2]
                if is_archive:
                    raw = f"Archived {'completed' if checkbox[1].lower() == 'x' else 'incomplete'} work item: {raw}"
                else:
                    item_state = checkbox[1].lower()
            entry_tags = unique(subject + kinds(raw, scope))
            if scope and scope != title:
                raw = f"{scope}: {raw}"
            add(source, day, entry_tags, raw, line_offset + start + 1, line_offset + position, state=item_state, basis=basis)

    for name, source in sources.items():
        if name == archived_source.row["source"]:
            # The collection itself and each of its 72 logical records remain addressable.
            add(source, date(2026, 10, 20), ["process"], "Original collection of 72 task snapshots before consolidation.", 1, len(archived_text.splitlines()), title="Document", basis="explicit consolidation date")
            for match in re.finditer(r"<!-- BEGIN tasks/(\d{8}-\d{6})/TASK.md -->\n(.*?)<!-- END tasks/\1/TASK.md -->", archived_text, re.S):
                offset = archived_text[:match.start(2)].count("\n")
                extract(source, match[2], offset, match[1], True)
        elif Path(name).suffix == ".md":
            extract(source, source.data.decode("utf-8-sig"))
        else:
            day, basis = date_basis(source)
            subject = subject_by_id.get(name.split("/")[1], ["spec"] if "/spec/" in name or "/new_spec/" in name else ["process"])
            add(source, day, subject, f"Artifact: {Path(name).name}. Date basis: {basis}.", 1, max(1, source.data.count(b"\n")), title="Artifact", basis=basis)

    for old_id, current in current_records.items():
        if old_id in previous_records:
            previous = previous_records[old_id]
            assert current.day >= previous.day, (old_id, current.day, previous.day)
            previous.tags = unique(previous.tags + ["superseded"])
            current.relation = previous

    entries.sort(key=lambda entry: (entry.day, entry.source, entry.start, 0 if entry.old_id else 1))
    counts = {}
    for entry in entries:
        entry.index = counts.get(entry.day, 0)
        counts[entry.day] = entry.index + 1
    mapping = {**previous_records, **current_records}
    unresolved = []

    def repaired_target(target: str, source_name: str, target_file: str) -> str:
        if not target or target.startswith(("#", "/", "mailto:", "http:", "https:")):
            return target
        path, hashmark, fragment = target.partition("#")
        lookup = posixpath.normpath(posixpath.join(posixpath.dirname(source_name), path))
        resolved = aliases.get(lookup)
        if resolved is None:
            # Older flattened files were originally relative to their task folder.
            leaf = Path(source_name).name
            if source_name.startswith("archive/20261020-000000/") and "--" in leaf:
                original = f"tasks/{leaf.split('--', 1)[0]}/{leaf.split('--', 1)[1]}"
                lookup = posixpath.normpath(posixpath.join(posixpath.dirname(original), path))
                resolved = aliases.get(lookup)
        if resolved is None and (ROOT / lookup).exists() and lookup not in aliases:
            resolved = lookup
        if resolved is None:
            if "/" in path or re.search(r"\.[A-Za-z0-9]+$", path) or path.startswith("TODO:"):
                unresolved.append({"source": source_name, "target": target})
            return target
        if resolved.startswith("https://"):
            return resolved + (hashmark + fragment if hashmark else "")
        relative = posixpath.relpath(resolved, posixpath.dirname(target_file))
        return relative + (hashmark + fragment if hashmark else "")

    def repair_links(text: str, source_name: str, target_file: str) -> str:
        lines = []
        fence = ""
        for line in text.splitlines(keepends=True):
            marker = re.match(r"^\s*(`{3,}|~{3,})", line)
            if marker:
                if not fence:
                    fence = marker[1]
                elif marker[1].startswith(fence):
                    fence = ""
                lines.append(line)
            elif fence:
                lines.append(line)
            else:
                # Preserve inline code, including calculus applications resembling links.
                pattern = r"(`+)[^`]*?\1|(?<!!)\[([^\]\n]*)\]\(([^()\s]+)\)"
                lines.append(re.sub(pattern, lambda match: match[0] if match[1] else f"[{match[2]}]({repaired_target(match[3], source_name, target_file)})", line))
        return "".join(lines)

    def replace_id(match: re.Match[str], old: bool) -> str:
        records = previous_records if old else mapping
        target = records.get(match[0]) or mapping.get(match[0])
        return f"({target.day:%d.%m.%y}, {target.index})" if target else match[0]

    def render(entry: Entry) -> str:
        body = entry.body
        # Inline explicit paths and Markdown links point to the relocated artifacts.
        body = repair_links(body, entry.source, "devlog/log.md")
        for old, target in sorted(aliases.items(), key=lambda pair: -len(pair[0])):
            body = body.replace(old, target)
        body = re.sub(r"(?<![\w/-])\d{8}-\d{6}(?![\w/-])", lambda match: replace_id(match, entry.source.startswith("archive/")), body)
        relation = ""
        if entry.relation is not None:
            target = entry.relation
            relation = f'Supersession for ({target.day:%d.%m.%y}, {target.index}) "{target.title}". '
        source = sources[entry.source]
        locator = f"{source.target}:{entry.start}"
        prefix = f"- [{entry.state}] " if entry.state else "- "
        tags = "(" + " ".join(f"#{tag}" for tag in entry.tags) + ")"
        return f"{prefix}{tags} {entry.title}. {relation}{body} Source: `{locator}`."

    if STAGE.exists():
        shutil.rmtree(STAGE)
    STAGE.mkdir()
    for source in sources.values():
        content = source.data
        if Path(source.target).suffix == ".md":
            original = content.decode("utf-8")
            content = repair_links(original, source.row["source"], source.target).encode("utf-8")
        path = STAGE / source.target
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        path.chmod(source.row["source_mode"])
        source.row.update(artifact=source.target, artifact_sha256=hashlib.sha256(content).hexdigest(), entry_ids=[entry.identity for entry in source.entries], blocks=[{"id": entry.identity, "lines": [entry.start, entry.end], "date_basis": entry.basis} for entry in source.entries])

    sections = []
    for day in sorted(counts):
        sections.append(f"### {day:%d.%m.%y}\n" + "\n".join(render(entry) for entry in entries if entry.day == day) + "\n")
    preferences = json.loads((ROOT / "devlog/preferences.json").read_text())
    chunks = []
    chunk = ""
    for section in sections:
        if chunk and len(chunk.encode("utf-8")) >= preferences["rotation"]["max_bytes"]:
            chunks.append(chunk)
            chunk = ""
        chunk += ("\n" if chunk else "") + section
    if chunk:
        chunks.append(chunk)
    for ordinal, chunk in enumerate(reversed(chunks)):
        name = "devlog/log.md" + (f".{ordinal}" if ordinal else "")
        path = STAGE / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(chunk)

    manifest = {"source_count": len(sources), "entry_count": len(entries), "date_count": len(counts), "files": [source.row for source in sources.values()], "legacy_ids": {old: entry.identity for old, entry in mapping.items()}, "unresolved_links": list({(item["source"], item["target"]): item for item in unresolved}.values())}
    path = STAGE / AUDIT / "manifest.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    print(f"Prepared {len(entries)} entries in {len(counts)} date sections and {len(chunks)} log files; {len(sources)} artifacts.")
    print(f"Unresolved original Markdown links: {len(manifest['unresolved_links'])}; see manifest.")
    print(f"Staging: {STAGE}")
    if sys.argv[1:] == ["apply"]:
        for source in sources.values():
            path = ROOT / source.row["source"]
            assert hashlib.sha256(path.read_bytes()).hexdigest() == source.row["source_sha256"], f"Source changed during migration: {path}"
            assert not (ROOT / source.target).exists(), f"Destination already exists: {source.target}"
        for path in sorted(STAGE.rglob("*")):
            if path.is_file():
                target = ROOT / path.relative_to(STAGE)
                assert not target.exists(), f"Refusing to overwrite {target}"
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, target)
        # All byte-preserving originals and relocated artifacts exist before old paths disappear.
        for source in sources.values():
            target = ROOT / source.target
            assert hashlib.sha256(target.read_bytes()).hexdigest() == source.row["artifact_sha256"]
        for source in sources.values():
            (ROOT / source.row["source"]).unlink()
        for folder in ("archive", "tasks"):
            for directory in sorted((ROOT / folder).rglob("*"), reverse=True):
                if directory.is_dir():
                    directory.rmdir()
            (ROOT / folder).rmdir()
        print("Published log and artifacts; removed old source paths after hash verification.")


if __name__ == "__main__":
    main()
