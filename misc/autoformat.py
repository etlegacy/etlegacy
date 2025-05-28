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

# Constants
INCLUDED_EXTS = {".c", ".cpp", ".glsl", ".h", ".py"}
EXCLUDED_PATHS = [
    "src/Omnibot/",
    "src/game/g_etbot_interface.cpp",
    "src/qcommon/crypto/sha-1/",
]


def change_to_repo_root():
    """Change working directory to the repository root."""
    script_path = Path(__file__).resolve().parent
    os.chdir(script_path.parent)


def get_git_changed_files(rev=None):
    """Get changed files from git diff."""
    if rev:
        print(f"Checking specific rev '{rev}'...")
        cmd = ["git", "diff", "--name-only", f"{rev}..{rev}^1"]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, check=True, text=True)
        files = result.stdout.strip().splitlines()
    else:
        print("Checking staged and unstaged changes...")
        staged = subprocess.check_output(
            ["git", "diff", "--name-only", "--cached"], text=True
        ).splitlines()
        unstaged = subprocess.check_output(
            ["git", "diff", "--name-only"], text=True
        ).splitlines()
        files = sorted(set(staged + unstaged))

    # Fallback to previous commit if no staged changes
    if not rev and not files:
        print("No staged or unstaged files found, falling back to previous commit...")
        cmd = ["git", "diff", "--name-only", "HEAD~1", "HEAD"]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, check=True, text=True)
        files = result.stdout.strip().splitlines()

    return files


def filter_target_files(files):
    """Filter out excluded files and non-target extensions."""
    filtered = []
    for path in files:
        if any(path.startswith(excl) for excl in EXCLUDED_PATHS):
            print(f"✋ {path}")
            continue
        if Path(path).suffix in INCLUDED_EXTS:
            filtered.append(path)
        else:
            print(f"✋ {path}")
    return filtered


def process_file(path):
    """Run uncrustify for C/C++ files or black for Python files and rewrite the file if formatting changed."""
    ext = Path(path).suffix

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
                print(f"✿ Formatting {path} (C/C++)")
                with open(path, "w", encoding="utf-8", errors="ignore") as f:
                    f.write(formatted)
            else:
                print(f"✓ {path} (C/C++ already formatted)")

        case ".py":
            # Check if formatting would change anything
            check_proc = subprocess.run(
                ["black", "--check", "--quiet", path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            if check_proc.returncode == 0:
                print(f"✓ {path} (Python already formatted)")
            else:
                print(f"✿ Formatting {path} (Python)")
                subprocess.run(["black", path], check=True)

        case _:
            print(f"✋ Skipping unsupported file type: {path}")


def main():
    change_to_repo_root()
    rev = sys.argv[1] if len(sys.argv) > 1 else None
    changed_files = get_git_changed_files(rev)
    target_files = filter_target_files(changed_files)

    had_errors = False
    with ThreadPoolExecutor(max_workers=cpu_count()) as executor:
        futures = {executor.submit(process_file, f): f for f in target_files}
        for future in as_completed(futures):
            try:
                future.result()
            except Exception as e:
                print(f"❌ Error processing {futures[future]}: {e}", file=sys.stderr)
                had_errors = True

    if had_errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
