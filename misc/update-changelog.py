#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Tuple


REPO_URL = "https://github.com/etlegacy/etlegacy.wiki.git"
REPO_FILE_PATH = "./Changelog.md"  # path inside the repo (as expected by fetch-single-file-from-git-repo.py)


def die(msg: str, code: int = 1) -> None:
    print(msg, file=sys.stderr)
    raise SystemExit(code)


def log(msg: str) -> None:
    print(msg, file=sys.stderr, flush=True)


def run_fetch(script_dir: Path) -> str:
    """
    Runs ./fetch-single-file-from-git-repo.py "<repo>" "<file>" in script_dir.
    If it fails, exits with the same return code.
    Returns stdout as text.
    """
    fetch_script = script_dir / "fetch-single-file-from-git-repo.py"
    if not fetch_script.is_file():
        die(f"Error: expected fetch script at: {fetch_script}")

    log(f"[1/4] Fetching changelog via {fetch_script.name} ...")
    cmd = [sys.executable, str(fetch_script), REPO_URL, REPO_FILE_PATH]
    proc = subprocess.run(
        cmd,
        cwd=str(script_dir),
        text=True,
        capture_output=True,
    )
    if proc.returncode != 0:
        # Preserve stderr for debugging; also include stdout in case it matters.
        log("[!] Fetch failed. Output follows:")
        if proc.stdout:
            print(proc.stdout, end="", file=sys.stderr)
        if proc.stderr:
            print(proc.stderr, end="", file=sys.stderr)
        raise SystemExit(proc.returncode)

    log(f"[1/4] Fetch OK ({len(proc.stdout)} bytes).")
    return proc.stdout


_heading_re = re.compile(r"^##\s+(.+?)\s*$")


def sanitize_filename(stem: str) -> str:
    # Keep common safe chars; replace everything else with "_"
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "_", stem).strip("._-")
    return cleaned or "unknown"


def header_to_stem(header_text: str) -> str:
    """
    Given the text after '## ', derive the output filename stem.
    Example: '2.83.2 - Mom ... (released ...)' -> '2.83.2'
    """
    # Prefer the first whitespace-delimited token (matches your examples).
    token = header_text.strip().split()[0]
    return sanitize_filename(token)


def split_by_h2_headers(markdown: str) -> List[Tuple[str, str]]:
    """
    Splits markdown into sections keyed by each '## ...' header.
    Returns list of (stem, content) in encounter order.
    Content excludes the header line itself.
    """
    lines = markdown.splitlines()
    sections: List[Tuple[str, List[str]]] = []

    current_stem: str | None = None
    current_lines: List[str] = []

    for line in lines:
        m = _heading_re.match(line)
        if m:
            # Close previous section (if any)
            if current_stem is not None:
                sections.append((current_stem, current_lines))
            # Start new section
            header_text = m.group(1)
            current_stem = header_to_stem(header_text)
            current_lines = []
            continue

        if current_stem is not None:
            current_lines.append(line)

    # Close last section
    if current_stem is not None:
        sections.append((current_stem, current_lines))

    # Normalize leading/trailing blank lines in each section
    out: List[Tuple[str, str]] = []
    for stem, content_lines in sections:
        # Strip leading/trailing empty lines but keep internal structure
        start = 0
        end = len(content_lines)
        while start < end and content_lines[start].strip() == "":
            start += 1
        while end > start and content_lines[end - 1].strip() == "":
            end -= 1
        body = "\n".join(content_lines[start:end])
        out.append((stem, body))
    return out


def write_sections(output_root: Path, sections: List[Tuple[str, str]]) -> List[Path]:
    """
    Writes the split sections as .md files under output_root.
    Returns the list of written file paths (absolute Paths), in encounter order.
    """
    output_root.mkdir(parents=True, exist_ok=True)

    written: List[Path] = []
    used: Dict[str, int] = {}
    for stem, body in sections:
        # If duplicate stems occur, suffix them deterministically: 2.83.md, 2.83_2.md, ...
        n = used.get(stem, 0) + 1
        used[stem] = n
        final_stem = stem if n == 1 else f"{stem}_{n}"

        out_path = (output_root / f"{final_stem}.md").resolve()
        with open(out_path, "w", encoding="utf-8", newline="\n") as f:
            if body:
                f.write(body)
            f.write("\n")

        written.append(out_path)

    return written


def run_converter(script_dir: Path, files: List[Path]) -> None:
    """
    Runs: go run ./main.go <args> inside ./changelog-markdown-converter/
    with args = the written files.
    If it fails, exits with that return code.
    """
    if not files:
        die("No output files to convert.", 2)

    converter_dir = script_dir / "changelog-markdown-converter"
    if not converter_dir.is_dir():
        die(f"Error: expected converter dir at: {converter_dir}")

    cmd = ["go", "run", "./main.go", *[str(p) for p in files]]
    log(f"[3/4] Running converter: {' '.join(cmd)} (cwd={converter_dir})")
    proc = subprocess.run(
        cmd,
        cwd=str(converter_dir),
        text=True,
    )
    if proc.returncode != 0:
        log("[!] Converter failed.")
        raise SystemExit(proc.returncode)
    log("[3/4] Converter OK.")


def remove_split_files(files: List[Path]) -> None:
    """
    Removes the split .md files that were created earlier.
    """
    log("[4/4] Removing split .md files ...")
    failed: List[Tuple[Path, str]] = []

    for p in files:
        try:
            # Be defensive: only remove regular .md files.
            if p.suffix != ".md":
                continue
            if not p.exists():
                continue
            if p.is_dir():
                failed.append((p, "is a directory"))
                continue
            p.unlink()
        except Exception as e:
            failed.append((p, str(e)))

    if failed:
        log("[!] Some files could not be removed:")
        for p, err in failed:
            log(f"    - {p}: {err}")
        raise SystemExit(3)

    log("[4/4] Removal OK.")


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Fetch ET:Legacy wiki Changelog.md, split into per-version markdown files, run converter, then delete the split files.",
    )
    ap.add_argument(
        "--output-root",
        default="../etmain/changelogs/",
        help="Directory to write output files into.",
    )
    args = ap.parse_args()

    script_dir = Path(__file__).resolve().parent
    os.chdir(script_dir)

    output_root = Path(args.output_root).expanduser().resolve()

    text = run_fetch(script_dir)

    log("[2/4] Splitting changelog into per-version sections ...")
    sections = split_by_h2_headers(text)
    if not sections:
        die("No '## ...' sections found in fetched changelog.", 2)
    log(f"[2/4] Split OK ({len(sections)} sections).")

    log(f"[2/4] Writing {len(sections)} files under: {output_root}")
    written_files = write_sections(output_root, sections)
    log(f"[2/4] Wrote {len(written_files)} files.")

    # Convert first, then remove the split .md files (as requested)
    run_converter(script_dir, written_files)
    remove_split_files(written_files)

    log("Done.")


if __name__ == "__main__":
    main()
