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


@dataclass
class Error:
    path: Path
    reason: str
    diff: Optional[str] = None

    def __str__(self):
        msg = f"> {self.path}\n| {self.reason}"
        if self.diff and SHOW_DIFF:
            msg += f"\n```\n{self.diff}\n```"
        return msg


def run_git_command(cmd: List[str]) -> List[str]:
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True
    )
    return result.stdout.strip().splitlines()


def get_changed_files(commit_hash: str) -> List[str]:
    return run_git_command(
        ["git", "diff-tree", "--no-commit-id", "--name-only", "-r", commit_hash]
    )


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


def check_commit(commit_hash: str) -> List[Error]:
    errors = []
    for file in get_changed_files(commit_hash):
        path = ROOT_DIR / file
        if not path.exists():
            print(f"? Skipping missing: {file}")
            continue

        print(f"* {file}")
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

    total_errors = 0
    for i, commit in enumerate(get_commits(args.commit_hash)):
        if i > 0:
            print("======")
        errors = check_commit(commit)
        if errors:
            print("------")
            for error in errors:
                print(error, file=sys.stderr)
            total_errors += len(errors)

    if total_errors:
        print(f"\nDetected {total_errors} issue(s)!")
        sys.exit(1)


def cli():
    import argparse

    parser = argparse.ArgumentParser(
        description="Check files in commit(s) for formatting or asset issues."
    )
    parser.add_argument(
        "commit_hash", nargs="?", help="Compare with this commit (defaults to HEAD)"
    )
    parser.add_argument(
        "-d",
        "--diff",
        action="store_true",
        help="Show unified diff output for uncrustify",
    )
    args = parser.parse_args()
    main(args)


if __name__ == "__main__":
    cli()
