#!/usr/bin/env python3
"""
struct_padding_report.py

Analyze C structs for wasted space (padding/alignment) using python-clang (libclang).

Usage:
  ./struct_padding_report.py path/to/file.c -- -Iinclude -DMACRO=1 --target=x86_64-linux-gnu
  ./struct_padding_report.py path/to/file.h -- -Iinclude

Notes:
- Layout depends on target/ABI and compile flags. Pass the same flags you use to build.
- By default, only structs *defined in the main input file* are reported (not in included headers).
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Tuple

from clang import cindex


@dataclass(frozen=True)
class StructReport:
    usr: str
    name: str
    file: str
    line: int

    size_bytes: int
    align_bytes: int

    padding_bits: int
    tail_padding_bits: Optional[int]
    max_internal_gap_bits: Optional[int]

    last_field: Optional[str]
    last_field_end_bits: Optional[int]

    fields_count: int


def _realpath(p: str) -> str:
    return os.path.realpath(os.path.expanduser(p))


def _fmt_bits(bits: Optional[int]) -> str:
    if bits is None:
        return "n/a"
    b, r = divmod(bits, 8)
    if r == 0:
        return f"{b}B"
    return f"{b}B+{r}b"


def _fmt_padding(bits: int) -> str:
    b, r = divmod(bits, 8)
    if r == 0:
        return f"{b}B"
    return f"{b}B+{r}b"


def _has_arg_prefix(args: List[str], prefixes: Tuple[str, ...]) -> bool:
    for a in args:
        for p in prefixes:
            if a == p or a.startswith(p):
                return True
    return False


def _clang_default_args_for_path(path: str, user_args: List[str]) -> List[str]:
    # Only add defaults if user didn't specify them.
    args = list(user_args)

    ext = os.path.splitext(path)[1].lower()
    if not _has_arg_prefix(args, ("-x",)):
        if ext in (".h", ".hh", ".hpp", ".hxx"):
            args = ["-x", "c-header"] + args
        else:
            args = ["-x", "c"] + args

    # For C, default to c11 if no -std= provided.
    if not _has_arg_prefix(args, ("-std=",)):
        args = ["-std=c11"] + args

    return args


def _walk(cursor: cindex.Cursor) -> Iterable[cindex.Cursor]:
    yield cursor
    for ch in cursor.get_children():
        yield from _walk(ch)


def _build_typedef_map(tu: cindex.TranslationUnit) -> Dict[str, str]:
    """
    Map record USR -> typedef name for cases like:
      typedef struct { ... } Foo;
    """
    m: Dict[str, str] = {}
    for cur in _walk(tu.cursor):
        if cur.kind == cindex.CursorKind.TYPEDEF_DECL:
            try:
                ut = cur.underlying_typedef_type
                if ut is None:
                    continue
                if ut.kind == cindex.TypeKind.RECORD:
                    decl = ut.get_declaration()
                    if decl is not None:
                        usr = decl.get_usr()
                        if usr and cur.spelling:
                            # Prefer the first typedef name we see for that record.
                            m.setdefault(usr, cur.spelling)
            except Exception:
                continue
    return m


def _in_main_file(cur: cindex.Cursor, main_file: str) -> bool:
    try:
        lf = cur.location.file
        if lf is None:
            return False
        return _realpath(lf.name) == main_file
    except Exception:
        return False


def _field_size_bits(field: cindex.Cursor) -> int:
    """
    Return field "occupied bits" for padding calculation.

    - Non-bitfields: sizeof(type)*8 (flexible/incomplete -> 0)
    - Bitfields: declared bit-width
    """
    try:
        if (
            field.kind == cindex.CursorKind.FIELD_DECL
            and getattr(field, "is_bitfield", lambda: False)()
        ):
            w = field.get_bitfield_width()
            return max(0, int(w))
    except Exception:
        pass

    try:
        sz = field.type.get_size()
        if sz is None or int(sz) < 0:
            return 0
        return int(sz) * 8
    except Exception:
        return 0


def _field_offset_bits(record_type: cindex.Type, field: cindex.Cursor) -> Optional[int]:
    """
    Best-effort bit offset for a field within a record.

    libclang exposes offsets primarily by field *name* (Type.get_offset("field")).
    Unnamed bitfields or exotic cases may not be retrievable; then we return None.
    """
    # 1) Try Type.get_offset(field_name) when field has a spelling.
    try:
        if field.spelling:
            off = record_type.get_offset(field.spelling)
            if off is not None:
                off_i = int(off)
                if off_i >= 0:
                    return off_i
    except Exception:
        pass

    # 2) Some bindings expose cursor.get_field_offsetof() (varies by version); try it.
    try:
        fn = getattr(field, "get_field_offsetof", None)
        if callable(fn):
            off = fn()
            if off is not None:
                off_i = int(off)
                if off_i >= 0:
                    return off_i
    except Exception:
        pass

    return None


def _struct_name(cur: cindex.Cursor, typedef_map: Dict[str, str]) -> str:
    usr = cur.get_usr() or ""
    if cur.spelling:
        return cur.spelling
    if usr and usr in typedef_map:
        return typedef_map[usr]
    # Fallback: anonymous struct with location.
    try:
        f = cur.location.file.name if cur.location.file else "<?>"
        l = cur.location.line
        return f"<anonymous@{os.path.basename(f)}:{l}>"
    except Exception:
        return "<anonymous>"


def analyze_struct(
    cur: cindex.Cursor, typedef_map: Dict[str, str]
) -> Optional[StructReport]:
    # Only real struct definitions.
    if cur.kind != cindex.CursorKind.STRUCT_DECL or not cur.is_definition():
        return None

    try:
        rt = cur.type
        size_bytes = int(rt.get_size())
        align_bytes = int(rt.get_align())
        if size_bytes < 0 or align_bytes < 0:
            return None
    except Exception:
        return None

    # Gather fields
    try:
        fields = [f for f in rt.get_fields()]
    except Exception:
        fields = []

    if not fields:
        return None

    # Total padding via: struct_size_bits - sum(field_occupied_bits)
    occupied_bits = 0
    for f in fields:
        occupied_bits += _field_size_bits(f)

    struct_size_bits = size_bytes * 8
    padding_bits = max(0, struct_size_bits - occupied_bits)

    # Best-effort internal gaps / tail padding via offsets
    offsets: List[Tuple[int, int, str]] = []  # (start_bit, end_bit, field_name)
    for f in fields:
        start = _field_offset_bits(rt, f)
        if start is None:
            offsets = []
            break
        sz_bits = _field_size_bits(f)
        end = start + sz_bits
        offsets.append((start, end, f.spelling or "<unnamed>"))

    tail_padding_bits: Optional[int] = None
    max_internal_gap_bits: Optional[int] = None
    last_field: Optional[str] = None
    last_end_bits: Optional[int] = None

    if offsets:
        offsets.sort(key=lambda t: (t[0], t[1]))
        max_gap = 0
        prev_end = offsets[0][1]
        for start, end, _nm in offsets[1:]:
            if start > prev_end:
                max_gap = max(max_gap, start - prev_end)
            prev_end = max(prev_end, end)
        last_field = offsets[-1][2]
        last_end_bits = offsets[-1][1]
        tail_padding_bits = max(0, struct_size_bits - last_end_bits)
        max_internal_gap_bits = max_gap

    # Location
    try:
        file = cur.location.file.name if cur.location.file else "<?>"
        line = int(cur.location.line)
    except Exception:
        file, line = "<?>", 0

    usr = cur.get_usr() or ""
    name = _struct_name(cur, typedef_map)

    return StructReport(
        usr=usr,
        name=name,
        file=file,
        line=line,
        size_bytes=size_bytes,
        align_bytes=align_bytes,
        padding_bits=padding_bits,
        tail_padding_bits=tail_padding_bits,
        max_internal_gap_bits=max_internal_gap_bits,
        last_field=last_field,
        last_field_end_bits=last_end_bits,
        fields_count=len(fields),
    )


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(
        description="Find C structs with the most wasted space due to padding/alignment (libclang layout).",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    ap.add_argument("path", help="Path to a .c or .h file")
    ap.add_argument(
        "--all-includes",
        action="store_true",
        help="Include structs defined in included headers too (can be noisy).",
    )
    ap.add_argument(
        "--top",
        type=int,
        default=0,
        help="Show only top N results (0 = show all).",
    )
    ap.add_argument(
        "--min-size",
        type=int,
        default=1,
        help="Ignore structs smaller than this many bytes.",
    )
    ap.add_argument(
        "clang_args",
        nargs=argparse.REMAINDER,
        help="Extra clang args after '--', e.g. -- -Iinclude -DM=1 --target=...",
    )

    ns = ap.parse_args(argv[1:])
    path = _realpath(ns.path)
    if not os.path.isfile(path):
        print(f"ERROR: not a file: {path}", file=sys.stderr)
        return 2

    clang_args = list(ns.clang_args)
    if clang_args and clang_args[0] == "--":
        clang_args = clang_args[1:]
    clang_args = _clang_default_args_for_path(path, clang_args)

    index = cindex.Index.create()
    try:
        tu = index.parse(
            path,
            args=clang_args,
            options=int(
                getattr(cindex.TranslationUnit, "PARSE_DETAILED_PROCESSING_RECORD", 0)
            ),
        )
    except Exception as e:
        print("ERROR: libclang failed to parse translation unit.", file=sys.stderr)
        print(f"  file: {path}", file=sys.stderr)
        print(f"  clang args: {clang_args}", file=sys.stderr)
        print(f"  error: {e}", file=sys.stderr)
        return 2

    typedef_map = _build_typedef_map(tu)

    main_file = path
    seen_usrs = set()
    reports: List[StructReport] = []

    for cur in _walk(tu.cursor):
        if cur.kind != cindex.CursorKind.STRUCT_DECL:
            continue
        if not cur.is_definition():
            continue

        if not ns.all_includes and not _in_main_file(cur, main_file):
            continue

        usr = cur.get_usr() or ""
        if usr and usr in seen_usrs:
            continue
        if usr:
            seen_usrs.add(usr)

        rep = analyze_struct(cur, typedef_map)
        if rep is None:
            continue
        if rep.size_bytes < ns.min_size:
            continue
        # If it has zero padding, it’s still valid, but might spam; keep it anyway.
        reports.append(rep)

    # Sort by most padding first; tie-breaker: more waste%, then bigger structs, then name.
    def sort_key(r: StructReport):
        waste_pct = (r.padding_bits / (r.size_bytes * 8)) if r.size_bytes > 0 else 0.0
        return (
            -(r.padding_bits // 8),
            -r.padding_bits,
            -waste_pct,
            -r.size_bytes,
            r.name,
        )

    reports.sort(key=sort_key)

    if ns.top and ns.top > 0:
        reports = reports[: ns.top]

    # Print
    print(f"# Struct padding report for: {path}")
    print(f"# clang args: {' '.join(clang_args)}")
    print()

    header = (
        f"{'Rank':>4}  {'Padding':>10}  {'Size':>6}  {'Align':>5}  {'Waste%':>7}  "
        f"{'Tail':>8}  {'MaxGap':>8}  {'Fields':>6}  Struct (file:line) [last field]"
    )
    print(header)
    print("-" * len(header))

    for i, r in enumerate(reports, 1):
        pad_str = _fmt_padding(r.padding_bits)
        waste_pct = (
            (r.padding_bits / (r.size_bytes * 8) * 100.0) if r.size_bytes > 0 else 0.0
        )
        tail_str = _fmt_bits(r.tail_padding_bits)
        gap_str = _fmt_bits(r.max_internal_gap_bits)

        loc = f"{os.path.basename(r.file)}:{r.line}"
        last = r.last_field if r.last_field is not None else "n/a"

        print(
            f"{i:>4}  {pad_str:>10}  {r.size_bytes:>6}  {r.align_bytes:>5}  {waste_pct:>6.1f}%  "
            f"{tail_str:>8}  {gap_str:>8}  {r.fields_count:>6}  {r.name} ({loc}) [{last}]"
        )

    if not reports:
        print("No struct definitions found (or all were filtered out).")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
