#!/usr/bin/env python3
# Checks repository changes for consistency/validity.
# Intended to be used for CI.
#
# REQUIRES: python uncrustify git
import os
import struct
import sys
import difflib
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import cpu_count
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional

from etl_lib import (
    CWD,
    ROOT_DIR,
    argparse_add_file_selection_params,
    get_current_git_branch,
    get_formatter,
    get_selected_files,
    is_excluded_path,
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
        result = run_command(["black", "--check", "--diff", str(path)], check=False)
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
    result = run_command(
        ["uncrustify", "-c", str(ROOT_DIR / "uncrustify.cfg"), "-f", str(path)],
        check=False,
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
    # shellcheck
    result = run_command(["shellcheck", str(path)], check=False)

    if result.returncode != 0:
        errors.append(
            Error(
                path=path,
                reason=f"Shellcheck failed",
                detail=result.stdout.strip(),
            )
        )

    # shfmt
    result = run_command(["shfmt", "--diff", str(path)], check=False)

    if result.returncode != 0:
        errors.append(
            Error(
                path=path,
                reason=f"Shfmt failed",
                detail=result.stdout.strip(),
            )
        )


def check_tga(path: Path, errors: List["Error"]):
    try:
        with open(path, "rb") as f:
            # TGA header is 18 bytes; image descriptor is the last byte (offset 17).
            f.seek(17)
            b = f.read(1)
            if len(b) != 1:
                raise ValueError(
                    "File too small to be a valid TGA (missing image descriptor)."
                )

            image_descriptor = b[0]
            top_left = (image_descriptor & 0x20) != 0  # bit 5 set => top-left origin

            if top_left:
                errors.append(
                    Error(
                        path,
                        "TGA file has top-left origin; bottom-left is required.",
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


def check_wav(path: Path, errors: List[Error]):
    try:
        with open(path, "rb") as f:
            riff_header = f.read(12)

            # WAV files are RIFF containers with a WAVE form type.
            if len(riff_header) != 12:
                errors.append(Error(path, "File too small to be a valid WAV."))
                return

            riff_id, _riff_size, wave_id = struct.unpack("<4sI4s", riff_header)
            if riff_id != b"RIFF" or wave_id != b"WAVE":
                errors.append(Error(path, "Not a valid RIFF/WAVE file."))
                return

            fmt = None

            # Find the mandatory fmt chunk and skip all unrelated chunks.
            while True:
                chunk_header = f.read(8)
                if not chunk_header:
                    break
                if len(chunk_header) != 8:
                    errors.append(Error(path, "Truncated WAV chunk header."))
                    return

                chunk_id, chunk_size = struct.unpack("<4sI", chunk_header)
                chunk_data = f.read(chunk_size)
                if len(chunk_data) != chunk_size:
                    errors.append(Error(path, "Truncated WAV chunk data."))
                    return

                # RIFF chunks are padded to even byte boundaries.
                if chunk_size & 1:
                    f.seek(1, os.SEEK_CUR)

                if chunk_id == b"fmt ":
                    fmt = chunk_data
                    break

            if fmt is None:
                errors.append(Error(path, "WAV file is missing fmt chunk."))
                return

            if len(fmt) < 16:
                errors.append(Error(path, "WAV fmt chunk is too small."))
                return

            (
                audio_format,
                channels,
                sample_rate,
                _byte_rate,
                _block_align,
                bits_per_sample,
            ) = struct.unpack("<HHIIHH", fmt[:16])

            if audio_format != 1:
                errors.append(Error(path, "WAV file must be uncompressed PCM."))
            if bits_per_sample != 16:
                errors.append(Error(path, "WAV file must be 16-bit."))
            if channels != 1:
                errors.append(Error(path, "WAV file must be mono."))
            if sample_rate not in (22050, 44100):
                errors.append(
                    Error(
                        path,
                        "WAV file must use a 22050 Hz or 44100 Hz sample rate. "
                        "This is not an engine limitation; other sample rates make "
                        "little practical sense for game content.",
                    )
                )
    except Exception as e:
        errors.append(Error(path, f"Failed to check WAV: {e}"))


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

    # formatter-backed file types (shared with autoformat)
    match get_formatter(path):
        case "uncrustify":
            check_uncrustify(path, errors)
        case "black":
            check_black(path, errors)
        case "shfmt":
            check_sh(path, errors)
        case "prettier":
            check_yml(path, errors)

    # asset checks, nothing to autoformat there
    match path.suffix.lower():
        case ".tga":
            check_tga(path, errors)
        case ".jpg" | ".jpeg":
            check_jpeg(path, errors)
        case ".wav":
            check_wav(path, errors)
    return errors


def check_changes(state: State):
    # assemble files
    changed_files = get_selected_files(state.args)

    if len(changed_files) < 1:
        print("No changes found.")
        return

    print("------------------------------------------")

    # collect the files that actually need checking
    to_check = []
    for path in sorted(list(changed_files)):
        relpath = path.relative_to(ROOT_DIR)

        # ignore files below certain directories
        if is_excluded_path(path):
            print(f"☇ Skipping ignored: {path.relative_to(ROOT_DIR)}")
            continue

        if not path.exists():
            print(f"☇ Skipping removed/missing: {path.relative_to(ROOT_DIR)}")
            continue

        to_check.append((path, relpath))

    # The checks are dominated by spawning external tools (uncrustify, black,
    # ...) per file, so run them in parallel. Threads are sufficient since the
    # work happens in subprocesses. map() preserves the (sorted) input order
    # so the output stays deterministic.
    with ThreadPoolExecutor(max_workers=cpu_count()) as executor:
        all_errors = list(executor.map(lambda x: check_file(x[0]), to_check))

    for (_path, relpath), errors in zip(to_check, all_errors):
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
    # with --all the commit is irrelevant, so never skip
    skip_changes = not args.all and args.commit_hash == current_git_branch

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
        description="Check files in commit(s) (or all git-tracked files with --all) for formatting or asset issues."
    )

    argparse_add_file_selection_params(parser)

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
