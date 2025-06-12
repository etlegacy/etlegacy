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

from etl_lib import get_changed_files, CWD, ROOT_DIR, argparse_add_commit_param

HIDE_DETAIL = False


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
                original, formatted, fromfile=str(path), tofile="uncrustified"
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
    return errors


def main(args):
    global HIDE_DETAIL
    HIDE_DETAIL = args.hide_details

    # assemble files
    changed_files = get_changed_files(args.commit_hash)

    if len(changed_files) < 1:
        print("No changes found.")
        quit(0)

    print("------------------------------------------")

    failed_files = []
    succeeded_files = []

    # check assembled files for errors
    for path in sorted(list(changed_files)):
        relpath = path.relative_to(ROOT_DIR)
        errors = check_file(path)
        if errors == None:
            pass
        elif errors:
            failed_files.append([relpath, errors])
        else:
            succeeded_files.append(relpath)

    for relpath in succeeded_files:
        print(f"✓ {relpath}")

    total_errors = 0
    if len(failed_files) > 0:
        print("------------------ERRORS------------------")

        for relpath, errors in failed_files:
            print(f"✗ {relpath}")
            for error in errors:
                print(error, file=sys.stderr)
                if args.github:
                    print(f"::error ::{error.path.relative_to(CWD)} - {error.reason}")
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
