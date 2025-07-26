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
from dataclasses import dataclass, field
from typing import List, Optional

from etl_lib import (
    CWD,
    ROOT_DIR,
    argparse_add_commit_param,
    get_changed_files,
    get_current_git_branch,
    rel_str,
    run_command,
)

HIDE_DETAIL = False


@dataclass
class State:
    args: list[str] = field(default_factory=list)
    failed_files: list[str] = field(default_factory=list)
    succeeded_files: list[str] = field(default_factory=list)


@dataclass
class Error:
    path: Path
    reason: str
    detail: Optional[str] = None

    def __str__(self):
        msg = f"{self.reason}"
        if self.detail and not HIDE_DETAIL:
            msg += f"\n```\n{self.detail}\n```"
        return msg


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
                    detail=result.stdout.strip() if not HIDE_DETAIL else None,
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
        detail = list(
            difflib.unified_diff(
                original, formatted, fromfile=rel_str(path), tofile=rel_str(path)
            )
        )

        if detail:
            errors.append(Error(path, "Not correctly uncrustified", "".join(detail)))
    except Exception as e:
        errors.append(Error(path, f"Failed to diff: {e}"))


def check_sh(path: Path, errors: List[Error]):
    result = subprocess.run(
        ["shellcheck", str(path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if result.returncode != 0:
        errors.append(
            Error(
                path=path,
                reason=f"Shellcheck failed",
                detail=result.stdout.strip(),
            )
        )
        return


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


def check_yml(path: Path, errors: List[Error]):
    result = run_command(["prettier", "--no-config", str(path)], check=False)

    if result.returncode != 0:
        errors.append(Error(path, f"Prettier failed: {result.stderr.strip()}"))
        return

    try:
        with open(path, "r", encoding="utf-8") as f:
            original = f.readlines()

        formatted = result.stdout.splitlines(keepends=True)
        detail = list(
            difflib.unified_diff(
                original, formatted, fromfile=rel_str(path), tofile=rel_str(path)
            )
        )

        if detail:
            errors.append(Error(path, "Not correctly prettied", "".join(detail)))
    except Exception as e:
        errors.append(Error(path, f"Failed to diff: {e}"))


def check_file(path: Path) -> Optional[List[Error]]:
    errors = []

    if not path.exists():
        print(f"☇ Skipping removed/missing: {path.relative_to(ROOT_DIR)}")
        return None

    match path.suffix.lower():
        case ".c" | ".cpp" | ".glsl" | ".h":
            check_uncrustify(path, errors)
        case ".tga":
            check_tga(path, errors)
        case ".jpg" | ".jpeg":
            check_jpeg(path, errors)
        case ".py":
            check_black(path, errors)
        case ".sh":
            check_sh(path, errors)
        case ".yml" | ".yaml":
            check_yml(path, errors)
    return errors


def check_changes(state: State):
    # assemble files
    changed_files = get_changed_files(state.args.commit_hash)

    if len(changed_files) < 1:
        print("No changes found.")
        return

    print("------------------------------------------")

    # check assembled files for errors
    for path in sorted(list(changed_files)):
        relpath = path.relative_to(ROOT_DIR)
        errors = check_file(path)
        if errors == None:
            pass
        elif errors:
            state.failed_files.append([relpath, errors])
        else:
            state.succeeded_files.append(relpath)


def check_global(state: State):
    result = run_command(["actionlint"], check=False)

    def check_global_actionlint():
        path = "[actionlint]"
        if result.returncode != 0:
            state.failed_files.append(
                [path, [Error(path, f"Actionlint failed:\n {result.stdout.strip()}")]]
            )
        else:
            state.succeeded_files.append(path)

    check_global_actionlint()


def main(args):
    global HIDE_DETAIL
    HIDE_DETAIL = args.hide_details

    state = State(args=args)

    current_git_branch = get_current_git_branch()
    skip_changes = args.commit_hash == current_git_branch

    if not skip_changes:
        check_changes(state)
    else:
        print("Skipping checking changes...")

    check_global(state)

    for relpath in state.succeeded_files:
        print(f"✓ {relpath}")

    total_errors = 0
    if len(state.failed_files) > 0:
        print("------------------ERRORS------------------")

        for relpath, errors in state.failed_files:
            print(f"✗ {relpath}")
            for error in errors:
                print(error, file=sys.stderr)
                if args.github:
                    path = error.path
                    if isinstance(path, Path):
                        path = error.path.relative_to(CWD)
                    print(f"::error ::{path} - {error.reason}")
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

    parser = argparse.ArgumentParser(
        description="Check files in commit(s) for formatting or asset issues."
    )

    argparse_add_commit_param(parser)

    parser.add_argument(
        "-hd",
        "--hide-details",
        dest="hide_details",
        action="store_true",
        help="Hide detailed output",
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
