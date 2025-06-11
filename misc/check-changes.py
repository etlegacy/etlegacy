#!/usr/bin/env python3
# Checks repository changes for consistency/validity.
# Intended to be used for CI.
#
# REQUIRES: python uncrustify git
import os
import sys
import subprocess
import difflib
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional

ROOT_DIR = Path(__file__).resolve().parent.parent
SHOW_DIFF = False

cwd = Path.cwd()


@dataclass
class Error:
    path: Path
    reason: str
    diff: Optional[str] = None

    def __str__(self):
        msg = f"{self.reason}"
        if self.diff and SHOW_DIFF:
            msg += f"\n```\n{self.diff}\n```"
        return msg


def run_git_command(cmd: List[str]) -> List[str]:
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True
    )
    return result.stdout.strip().splitlines()


def get_changed_files(commit_hash: str) -> List[Path]:
    print("", commit_hash)
    return [
        ROOT_DIR / path
        for path in run_git_command(
            ["git", "diff-tree", "--no-commit-id", "--name-only", "-r", commit_hash]
        )
    ]


def get_staged_unstaged_files() -> List[Path]:
    staged = run_git_command(["git", "diff", "--cached", "--name-only"])
    unstaged = run_git_command(["git", "diff", "--name-only"])
    return [ROOT_DIR / path for path in staged + unstaged]


def check_black(path: Path, errors: List[Error]):
    try:
        result = subprocess.run(
            ["black", "--check", "--diff", str(path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if result.returncode != 0:
            errors.append(
                Error(
                    path=path,
                    reason="Not formatted with black.",
                    diff=result.stdout.strip() if SHOW_DIFF else None,
                )
            )
    except Exception as e:
        errors.append(Error(path, f"Failed to check with black: {e}"))


def check_uncrustify(path: Path, errors: List[Error]):
    result = subprocess.run(
        ["uncrustify", "-c", str(ROOT_DIR / "uncrustify.cfg"), "-f", str(path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.returncode != 0:
        errors.append(Error(path, f"Uncrustify failed: {result.stderr.strip()}"))
        return

    try:
        with open(path, "r", encoding="utf-8") as f:
            original = f.readlines()

        formatted = result.stdout.splitlines(keepends=True)
        diff = list(
            difflib.unified_diff(
                original, formatted, fromfile=str(path), tofile="uncrustified"
            )
        )

        if diff:
            errors.append(Error(path, "Not correctly uncrustified", "".join(diff)))
    except Exception as e:
        errors.append(Error(path, f"Failed to diff: {e}"))


def check_tga(path: Path, errors: List[Error]):
    try:
        with open(path, "rb") as f:
            f.seek(16)
            if f.read(1) != b"\x20":
                errors.append(
                    Error(
                        path,
                        "TGA file has top-left origin, but bottom-left is required.",
                    )
                )
    except Exception as e:
        errors.append(Error(path, f"Failed to check TGA origin: {e}"))


def check_jpeg(path: Path, errors: List[Error]):
    try:
        with open(path, "rb") as f:
            data = f.read()
        if not data.startswith(b"\xff\xd8"):
            errors.append(Error(path, "Not a valid JPEG file."))
        elif data.count(b"\xff\xda") > 1:
            errors.append(Error(path, "Progressive JPEG not supported."))
    except Exception as e:
        errors.append(Error(path, f"Failed to check JPEG: {e}"))


def check_file(path: Path) -> List[Error]:
    errors = []

    if not path.exists():
        print(f"? Skipping missing: {path.relative_to(ROOT_DIR)}")
        return []

    match path.suffix.lower():
        case ".c" | ".h":
            check_uncrustify(path, errors)
        case ".tga":
            check_tga(path, errors)
        case ".jpg" | ".jpeg":
            check_jpeg(path, errors)
        case ".py":
            check_black(path, errors)
    return errors


def get_commits(base_commit: Optional[str]) -> List[str]:
    if base_commit:
        return run_git_command(["git", "rev-list", f"...{base_commit}"])
    return [run_git_command(["git", "rev-parse", "HEAD"])[0]]


def main(args):
    global SHOW_DIFF
    SHOW_DIFF = args.diff

    print("Considering changes towards", f"'{args.commit_hash}' ...")

    # assemble files
    changed_files = set()
    total_errors = 0

    staged_unstaged_files = get_staged_unstaged_files()
    if staged_unstaged_files:
        print(" picking up staged/unstaged files")
        changed_files.update(staged_unstaged_files)

    for i, commit in enumerate(get_commits(args.commit_hash)):
        changed_files.update(get_changed_files(commit))

    print("------------------------------------------")

    failed_files = []
    succeeded_files = []

    # check assembled files for errors
    for path in sorted(list(changed_files)):
        relpath = path.relative_to(ROOT_DIR)
        errors = check_file(path)
        if errors:
            failed_files.append([relpath, errors])
        else:
            succeeded_files.append(relpath)

    for relpath in succeeded_files:
        print(f"✓ {relpath}")

    if len(failed_files) > 0:
        print("------------------ERRORS------------------")

        for relpath, errors in failed_files:
            print(f"✗ {relpath}")
            for error in errors:
                print(error, file=sys.stderr)
                if args.github:
                    print(f"::error ::{error.path.relative_to(cwd)} - {error.reason}")
                total_errors += 1

    if total_errors:
        msg = f"\nFailed {total_errors} check"
        if total_errors > 1:
            msg += "s"
        msg += "!"
        print(msg)
        sys.exit(1)


def cli():
    import argparse

    # optionally read default master_branch from
    # 'etlegacy/.check-changes-master-branch'
    master_branch = "origin/master"
    master_branch_path = ROOT_DIR / ".check-changes-master-branch"
    if master_branch_path.exists():
        with open(master_branch_path, "r", encoding="utf-8") as f:
            master_branch = f.read().strip()

    parser = argparse.ArgumentParser(
        description="Check files in commit(s) for formatting or asset issues."
    )
    parser.add_argument(
        "commit_hash",
        default=master_branch,
        nargs="?",
        help="Compare with this commit (defaults to HEAD)",
    )
    parser.add_argument(
        "-d",
        "--diff",
        action="store_true",
        help="Show unified diff output for uncrustify",
    )
    parser.add_argument(
        "-gh",
        "--github",
        action="store_true",
        help="Print special messages that Github Actions integrates better for error reporting",
    )
    args = parser.parse_args()
    main(args)


if __name__ == "__main__":
    cli()
