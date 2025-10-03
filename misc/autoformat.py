#!/usr/bin/env python3
# Autoformats your local code.
#
# REQUIRES: python uncrustify git
import os
import sys
import difflib
import subprocess

from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from multiprocessing import cpu_count

from etl_lib import (
    get_changed_files,
    CWD,
    ROOT_DIR,
    argparse_add_commit_param,
    rel,
    run_command,
)

# Constants
INCLUDED_EXTS = {".c", ".cpp", ".h", ".py", ".sh", ".yml", ".yaml"}
EXCLUDED_PATHS = [
    "src/Omnibot/",
    "src/game/g_etbot_interface.cpp",
    "src/luasql/",
    "src/qcommon/crypto/sha-1/",
    "src/renderercommon/nanosvg/",
    "src/tools/shdr/tinydir.h",
]


def _excluded_abs_paths():
    root = Path(ROOT_DIR).resolve()
    return [(root / p).resolve() for p in EXCLUDED_PATHS]


def filter_target_files(files):
    """Filter out excluded files and non-target extensions."""
    excluded = _excluded_abs_paths()
    filtered = []
    for path in files:
        path = Path(path).resolve()
        if any(path.is_relative_to(excl) for excl in excluded):
            print(f"⏸ {rel(path)}")
            continue
        if path.suffix in INCLUDED_EXTS:
            filtered.append(path)
        else:
            print(f"⏸ {rel(path)}")
    return filtered


def list_all_target_files_from_root():
    """
    List all *git-tracked* files under the repo root that:
      - have an extension in INCLUDED_EXTS
      - are NOT under any EXCLUDED_PATHS

    Returns absolute Paths under ROOT_DIR so rel() works.
    """
    root = Path(ROOT_DIR).resolve()
    excluded = _excluded_abs_paths()

    # Build git pathspecs for the included extensions.
    # Using ':(glob)' ensures ** works reliably.
    pathspecs = [f":(glob)**/*{ext}" for ext in INCLUDED_EXTS]

    try:
        proc = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z", "--", *pathspecs],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
        entries = [e for e in proc.stdout.decode("utf-8").split("\x00") if e]
    except subprocess.CalledProcessError as e:
        print(
            f"❌ git ls-files failed ({e.returncode}). Falling back to filesystem scan.",
            file=sys.stderr,
        )
        entries = []

    targets = []

    if entries:
        # Convert to absolute paths and apply EXCLUDED_PATHS filter
        for rel_path in entries:
            p = (root / rel_path).resolve()
            if p.suffix not in INCLUDED_EXTS:
                continue
            if any(p.is_relative_to(excl) for excl in excluded):
                print(f"⏸ {rel(p)}")
                continue
            targets.append(p)
        return targets

    # Fallback: rglob (only used if git command failed)
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix not in INCLUDED_EXTS:
            continue
        if any(p.is_relative_to(excl) for excl in excluded):
            print(f"⏸ {rel(p)}")
            continue
        targets.append(p)

    return targets


def process_file(path) -> str:
    """Run uncrustify for C/C++ files or black for Python files and rewrite the file if formatting changed."""
    path = Path(path)
    ext = path.suffix
    match ext:
        case ".c" | ".cpp" | ".glsl" | ".h":
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                original = f.read()
            proc = subprocess.run(
                ["uncrustify", "-c", "uncrustify.cfg", "-f", str(path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True,
            )
            formatted = proc.stdout.decode("utf-8")
            if original != formatted:
                with open(path, "w", encoding="utf-8", errors="ignore") as f:
                    f.write(formatted)
                return f"✿ Formatting {rel(path)} (C/C++)"
            else:
                return f"✓ {rel(path)} (C/C++ already formatted)"
        case ".py":
            # Check if formatting would change anything
            check_proc = subprocess.run(
                ["black", "--check", "--quiet", str(path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            if check_proc.returncode == 0:
                return f"✓ {rel(path)} (Python already formatted)"
            else:
                run_command(["black", str(path)])
                return f"✿ Formatting {rel(path)} (Python)"
        case ".sh":
            # Check if formatting would change anything
            check_proc = subprocess.run(
                ["shfmt", "--diff", str(path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            if check_proc.returncode == 0:
                return f"✓ {rel(path)} (Shfmt already formatted)"
            else:
                run_command(["shfmt", "--write", str(path)])
                return f"✿ Formatting {rel(path)} (Shfmt)"
        case ".yml" | ".yaml":
            # Check if formatting would change anything
            check_proc = run_command(
                ["prettier", "--no-config", "--check", str(path)], check=False
            )
            if check_proc.returncode == 0:
                return f"✓ {rel(path)} (YAML already formatted)"
            else:
                run_command(["prettier", "--no-config", "--write", str(path)])
                return f"✿ Formatting {rel(path)} (YAML Prettier)"
        case _:
            return f"⏸ Skipping unsupported file type: {rel(path)}"


def main(args):
    os.chdir(ROOT_DIR)

    if args.all:
        print("------------------------------------------")
        target_files = list_all_target_files_from_root()
        if len(target_files) < 1:
            print("No target files found.")
            quit(0)
    else:
        changed_files = get_changed_files(args.commit_hash)
        if len(changed_files) < 1:
            print("No changes found.")
            quit(0)
        print("------------------------------------------")
        target_files = filter_target_files(changed_files)

    applied_format = []
    alread_formatted = []
    had_errors = False

    with ThreadPoolExecutor(max_workers=cpu_count()) as executor:
        futures = {executor.submit(process_file, f): f for f in target_files}
        for future in as_completed(futures):
            try:
                result = future.result()
                if result.startswith("✓"):
                    alread_formatted.append(result)
                elif result.startswith("✿"):
                    applied_format.append(result)
                elif result.startswith("⏸"):
                    alread_formatted.append(result)
                else:
                    assert False, f"Unknown result: ${result}"
            except Exception as e:
                print(f"❌ Error processing {futures[future]}: {e}", file=sys.stderr)
                had_errors = True

    for line in alread_formatted:
        print(line)

    if len(applied_format) > 0:
        print("")
        for line in applied_format:
            print(line)

    if had_errors:
        sys.exit(1)


def cli():
    import argparse

    parser = argparse.ArgumentParser(
        description="Autoformats your local changes (or all git-tracked files with --all) so that they pass 'check-changes'."
    )

    argparse_add_commit_param(parser)
    parser.add_argument(
        "--all",
        action="store_true",
        help="Format all git-tracked files from the repo root, respecting INCLUDED_EXTS and EXCLUDED_PATHS.",
    )
    args = parser.parse_args()
    main(args)


if __name__ == "__main__":
    cli()
