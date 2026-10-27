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
    get_formatter,
    get_selected_files,
    is_excluded_path,
    CWD,
    ROOT_DIR,
    argparse_add_file_selection_params,
    rel,
    run_command,
)


def filter_target_files(files, report_unsupported=True):
    """
    Filter out excluded files and non-target extensions.

    Skipped files are reported, except for unsupported file types if
    report_unsupported is False (there are far too many when looking at all
    tracked files).
    """
    filtered = []
    for path in sorted(Path(f).resolve() for f in files):
        if not get_formatter(path):
            if report_unsupported:
                print(f"⏸ {rel(path)}")
        elif is_excluded_path(path):
            print(f"⏸ {rel(path)}")
        else:
            filtered.append(path)
    return filtered


def process_file(path) -> str:
    """Run uncrustify for C/C++ files or black for Python files and rewrite the file if formatting changed."""
    path = Path(path)
    match get_formatter(path):
        case "uncrustify":
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
        case "black":
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
        case "shfmt":
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
        case "prettier":
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

    selected_files = get_selected_files(args)
    if len(selected_files) < 1:
        print("No changes found.")
        quit(0)
    print("------------------------------------------")
    target_files = filter_target_files(selected_files, report_unsupported=not args.all)

    # filter removed files
    target_files = [x for x in target_files if Path(x).exists()]

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

    argparse_add_file_selection_params(parser)
    args = parser.parse_args()
    main(args)


if __name__ == "__main__":
    cli()
