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
INCLUDED_EXTS = {".c", ".cpp", ".glsl", ".h", ".py", ".yml", ".yaml"}
EXCLUDED_PATHS = [
    "src/Omnibot/",
    "src/game/g_etbot_interface.cpp",
    "src/qcommon/crypto/sha-1/",
]


def filter_target_files(files):
    """Filter out excluded files and non-target extensions."""
    filtered = []
    for path in files:
        if any(path.is_relative_to(excl) for excl in [Path(p) for p in EXCLUDED_PATHS]):
            print(f"⏸ {rel(path)}")
            continue
        if path.suffix in INCLUDED_EXTS:
            filtered.append(path)
        else:
            print(f"⏸ {rel(path)}")
    return filtered


def process_file(path) -> str:
    """Run uncrustify for C/C++ files or black for Python files and rewrite the file if formatting changed."""
    ext = path.suffix

    match ext:
        case ".c" | ".cpp" | ".glsl" | ".h":
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                original = f.read()

            proc = subprocess.run(
                ["uncrustify", "-c", "uncrustify.cfg", "-f", path],
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
                ["black", "--check", "--quiet", path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            if check_proc.returncode == 0:
                return f"✓ {rel(path)} (Python already formatted)"
            else:
                run_command(["black", path])
                return f"✿ Formatting {rel(path)} (Python)"

        case ".yml" | ".yaml":
            # Check if formatting would change anything
            check_proc = run_command(
                ["prettier", "--no-config", "--check", path], check=False
            )
            if check_proc.returncode == 0:
                return f"✓ {rel(path)} (YAML already formatted)"
            else:
                run_command(["prettier", "--no-config", "--write", path])
                return f"✿ Formatting {rel(path)} (YAML Prettier)"

        case _:
            return f"⏸ Skipping unsupported file type: {rel(path)}"


def main(args):
    os.chdir(ROOT_DIR)

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
        description="Autoformats your local changes so that they pass 'check-changes'."
    )

    argparse_add_commit_param(parser)
    args = parser.parse_args()
    main(args)


if __name__ == "__main__":
    cli()
