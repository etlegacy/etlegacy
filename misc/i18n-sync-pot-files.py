#!/usr/bin/env python3
"""
Pot/PO Synchronization Tool

Purpose
- Keep each .pot template file in sync with the union of msgids found in its
  corresponding .po translations (optionally pruning stale .pot entries).

What it does
- For each mapping:
  - etmain/locale/etlegacy_client.pot  <->  etmain/locale/client/*.po
  - etmain/locale/etlegacy_mod.pot     <->  etmain/locale/mod/*.po
- Reads all .po files and collects their msgid entries (including msgctxt and
  msgid_plural, if present).
- Reads the .pot template and:
  - Adds new entries to the .pot for any msgid found in .po but missing from .pot.
  - Optionally (with --prune): removes .pot entries that do not exist in any .po.

How it works
- Parses .po/.pot files into logical entries separated by blank lines.
- Builds a key from (msgctxt, msgid, msgid_plural) to uniquely identify entries.
- Keeps the original .pot entry blocks for matches to preserve comments and
  formatting where possible.
- When adding an entry, writes a minimal .pot block with empty msgstr fields
  and a marker reference comment.
- Skips obsolete gettext entries (blocks starting with "#~").

Notes and limitations
- The .pot is treated as authoritative for existing comments/locations. Newly
  added entries do NOT copy translator comments or flags from .po files.
- This tool does not run xgettext or update source references from code.
- It assumes UTF-8 .po/.pot files and simple gettext formatting.
- It does not attempt to normalize or rewrap existing .pot formatting.

Usage
- python3 misc/i18-sync-pot-files.py
- python3 misc/i18-sync-pot-files.py --prune
- python3 misc/i18-sync-pot-files.py --dry-run
- python3 misc/i18-sync-pot-files.py --check
"""
from __future__ import annotations

import argparse
import ast
import os
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple


@dataclass(frozen=True)
class EntryKey:
    msgctxt: Optional[str]
    msgid: str
    msgid_plural: Optional[str]


@dataclass
class Entry:
    raw_lines: List[str]
    comments: List[str]
    msgctxt: Optional[str]
    msgid: str
    msgid_plural: Optional[str]
    msgstrs: Dict[Optional[int], str] = field(default_factory=dict)

    @property
    def key(self) -> EntryKey:
        return EntryKey(self.msgctxt, self.msgid, self.msgid_plural)

    @property
    def is_header(self) -> bool:
        # Standard gettext header entry: msgid ""
        return self.msgid == ""


def _unescape_po_string(quoted: str, *, path: str, lineno: int) -> str:
    """
    quoted includes surrounding quotes.
    Uses Python's string literal parser for escape handling.
    """
    try:
        return ast.literal_eval(quoted)
    except Exception as exc:
        raise ValueError(
            f"{path}:{lineno}: invalid PO string literal: {quoted!r}"
        ) from exc


def _escape_po_string(value: str) -> str:
    # Minimal safe escaping for gettext-style strings.
    return value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def _format_po_string(keyword: str, value: str) -> List[str]:
    """
    Format a gettext string field. If value contains newlines, emits the common
    multi-line style:
      keyword ""
      "line1\n"
      "line2"
    """
    if "\n" not in value:
        return [f'{keyword} "{_escape_po_string(value)}"']

    parts = value.split("\n")
    out = [f'{keyword} ""']
    for i, part in enumerate(parts):
        if i < len(parts) - 1:
            out.append(f'"{_escape_po_string(part)}\\n"')
        else:
            out.append(f'"{_escape_po_string(part)}"')
    return out


def _is_obsolete_block(block_lines: List[Tuple[int, str]]) -> bool:
    """
    Obsolete gettext entries start with "#~ ..." lines and should not be treated as live entries.
    """
    for _, ln in block_lines:
        if ln.strip() == "":
            continue
        return ln.lstrip().startswith("#~")
    return False


def _parse_entry(block_lines: List[Tuple[int, str]], *, path: str) -> Entry:
    comments: List[str] = []
    msgctxt: Optional[str] = None
    msgid: Optional[str] = None
    msgid_plural: Optional[str] = None
    msgstrs: Dict[Optional[int], str] = {}

    current_field: Optional[Tuple[str, Optional[int]]] = None

    for lineno, line in block_lines:
        if line.startswith("#"):
            comments.append(line)
            continue

        if line.startswith("msgctxt "):
            current_field = ("msgctxt", None)
            quoted = line.split(" ", 1)[1].strip()
            msgctxt = _unescape_po_string(quoted, path=path, lineno=lineno)
            continue

        if line.startswith("msgid_plural "):
            current_field = ("msgid_plural", None)
            quoted = line.split(" ", 1)[1].strip()
            msgid_plural = _unescape_po_string(quoted, path=path, lineno=lineno)
            continue

        if line.startswith("msgid "):
            current_field = ("msgid", None)
            quoted = line.split(" ", 1)[1].strip()
            msgid = _unescape_po_string(quoted, path=path, lineno=lineno)
            continue

        if line.startswith("msgstr["):
            # msgstr[0] "...", msgstr[1] "..."
            try:
                idx = int(line.split("]", 1)[0][7:])
            except Exception as exc:
                raise ValueError(
                    f"{path}:{lineno}: invalid msgstr index syntax: {line!r}"
                ) from exc
            current_field = ("msgstr", idx)
            quoted = line.split(" ", 1)[1].strip()
            msgstrs[idx] = _unescape_po_string(quoted, path=path, lineno=lineno)
            continue

        if line.startswith("msgstr "):
            current_field = ("msgstr", None)
            quoted = line.split(" ", 1)[1].strip()
            msgstrs[None] = _unescape_po_string(quoted, path=path, lineno=lineno)
            continue

        # Continuation of the last field: "...."
        if line.startswith('"') and current_field is not None:
            value = _unescape_po_string(line.strip(), path=path, lineno=lineno)
            field_name, field_idx = current_field
            if field_name == "msgctxt":
                msgctxt = (msgctxt or "") + value
            elif field_name == "msgid":
                msgid = (msgid or "") + value
            elif field_name == "msgid_plural":
                msgid_plural = (msgid_plural or "") + value
            elif field_name == "msgstr":
                msgstrs[field_idx] = msgstrs.get(field_idx, "") + value
            continue

        # Unknown/unsupported line types are ignored, but you may want to be strict.
        # For sanity, we do NOT fail hard here to avoid breaking on minor variations.

    if msgid is None:
        msgid = ""

    return Entry(
        raw_lines=[ln for _, ln in block_lines],
        comments=comments,
        msgctxt=msgctxt,
        msgid=msgid,
        msgid_plural=msgid_plural,
        msgstrs=msgstrs,
    )


def parse_po_file(path: str) -> List[Entry]:
    with open(path, "r", encoding="utf-8", newline="") as f:
        raw_lines = f.read().splitlines()

    entries: List[Entry] = []
    buf: List[Tuple[int, str]] = []

    for lineno_1based, line in enumerate(raw_lines, start=1):
        if line.strip() == "":
            if buf:
                if not _is_obsolete_block(buf):
                    entries.append(_parse_entry(buf, path=path))
                buf = []
        else:
            buf.append((lineno_1based, line))

    if buf:
        if not _is_obsolete_block(buf):
            entries.append(_parse_entry(buf, path=path))

    return entries


def build_pot_entry_from_po(entry: Entry) -> List[str]:
    """
    Build a minimal .pot entry.
    We intentionally DO NOT copy comments/flags from .po (e.g. '#, fuzzy', translator notes).
    """
    lines: List[str] = ["#: via misc/i18n-sync-pot-files.py"]

    if entry.msgctxt is not None:
        lines.extend(_format_po_string("msgctxt", entry.msgctxt))

    lines.extend(_format_po_string("msgid", entry.msgid))

    if entry.msgid_plural is not None:
        lines.extend(_format_po_string("msgid_plural", entry.msgid_plural))
        # Typical template emits at least 2 plural forms.
        lines.extend(_format_po_string("msgstr[0]", ""))
        lines.extend(_format_po_string("msgstr[1]", ""))
    else:
        lines.extend(_format_po_string("msgstr", ""))

    return lines


def _stable_key_sort(k: EntryKey) -> Tuple[str, str, str]:
    return (k.msgctxt or "", k.msgid, k.msgid_plural or "")


def sync_pot(
    pot_path: str,
    po_dir: str,
    *,
    prune: bool,
    dry_run: bool,
    verbose: bool,
) -> Tuple[int, int, int, bool]:
    # Deterministic file order
    po_files = sorted(
        os.path.join(po_dir, f) for f in os.listdir(po_dir) if f.endswith(".po")
    )

    if prune and not po_files:
        # Safety guard: pruning with no .po files would wipe almost everything.
        raise RuntimeError(
            f"{po_dir}: no .po files found; refusing to --prune to avoid wiping {pot_path}"
        )

    po_entries: List[Entry] = []
    for path in po_files:
        po_entries.extend(parse_po_file(path))

    # Union of PO keys (excluding header)
    po_keys = {e.key for e in po_entries if not e.is_header}

    pot_entries = parse_po_file(pot_path)

    kept_entries: List[Entry] = []
    removed = 0
    removed_keys: List[EntryKey] = []

    for e in pot_entries:
        if e.is_header:
            kept_entries.append(e)
            continue

        if prune:
            if e.key in po_keys:
                kept_entries.append(e)
            else:
                removed += 1
                removed_keys.append(e.key)
        else:
            kept_entries.append(e)

    # Keys currently present in kept pot (excluding header)
    kept_pot_keys = {e.key for e in kept_entries if not e.is_header}

    # Add missing entries, deduped by key (avoid duplicates from multiple languages)
    to_add_by_key: Dict[EntryKey, Entry] = {}
    for e in po_entries:
        if e.is_header:
            continue
        if e.key in kept_pot_keys:
            continue
        # Keep the first representative encountered (po_files are sorted)
        to_add_by_key.setdefault(e.key, e)

    # Deterministic addition ordering
    to_add_keys_sorted = sorted(to_add_by_key.keys(), key=_stable_key_sort)
    to_add = [to_add_by_key[k] for k in to_add_keys_sorted]

    # Build new pot content (preserving original raw blocks for kept entries)
    out_lines: List[str] = []
    for i, e in enumerate(kept_entries):
        out_lines.extend(e.raw_lines)
        if i != len(kept_entries) - 1:
            out_lines.append("")

    if to_add:
        if out_lines and out_lines[-1] != "":
            out_lines.append("")
        for idx, e in enumerate(to_add):
            out_lines.extend(build_pot_entry_from_po(e))
            if idx != len(to_add) - 1:
                out_lines.append("")

    new_text = "\n".join(out_lines) + "\n"

    with open(pot_path, "r", encoding="utf-8", newline="") as f:
        old_text = f.read()

    changed = new_text != old_text

    if verbose and prune and removed_keys:
        print(f"{pot_path}: removed keys:", file=sys.stderr)
        for k in sorted(removed_keys, key=_stable_key_sort):
            print(
                f"  - msgctxt={k.msgctxt!r} msgid={k.msgid!r} msgid_plural={k.msgid_plural!r}",
                file=sys.stderr,
            )

    if changed and not dry_run:
        tmp_path = pot_path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(new_text)
        os.replace(tmp_path, pot_path)

    return (len(kept_entries), removed, len(to_add), changed)


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(
        description="Sync .pot templates with union of msgids from .po files."
    )
    ap.add_argument(
        "--prune",
        action="store_true",
        help="Remove .pot entries that do not exist in any corresponding .po file (unsafe unless you really want this).",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Do not write files; only print what would change.",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="Exit non-zero if changes would be made (implies --dry-run).",
    )
    ap.add_argument(
        "--verbose",
        action="store_true",
        help="Print additional details (like removed keys when pruning).",
    )

    args = ap.parse_args(argv)

    if args.check:
        args.dry_run = True

    mappings = [
        ("etmain/locale/etlegacy_client.pot", "etmain/locale/client"),
        ("etmain/locale/etlegacy_mod.pot", "etmain/locale/mod"),
    ]

    any_changed = False

    for pot_path, po_dir in mappings:
        if not os.path.isfile(pot_path):
            print(f"Missing pot: {pot_path}", file=sys.stderr)
            return 1
        if not os.path.isdir(po_dir):
            print(f"Missing po dir: {po_dir}", file=sys.stderr)
            return 1

        try:
            kept, removed, added, changed = sync_pot(
                pot_path,
                po_dir,
                prune=args.prune,
                dry_run=args.dry_run,
                verbose=args.verbose,
            )
        except Exception as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 1

        any_changed = any_changed or changed
        suffix = (
            " (would change)"
            if (changed and args.dry_run)
            else (" (changed)" if changed else "")
        )
        print(
            f"{pot_path}: kept {kept} entries, removed {removed}, added {added}{suffix}"
        )

    if args.check and any_changed:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
