#!/usr/bin/env python
# Checks repository changes for consistency/validity.
# Intended to be used for CI.
#
# REQUIRES: python uncrustify diff git
import os
import subprocess
import sys

from pathlib import Path

from dataclasses import dataclass, field
from typing import *

# TODO - run checks in parallel
# TODO - skip vendored .c/.h files
# TODO - also check uncommitted local changes - currently only checks changes of git commits

show_diff = False
errors = []

rootd = Path(os.path.realpath(__file__)).absolute().parent.parent


@dataclass
class Error:
    path: Path
    reason: str
    diff: str = None

    def __str__(self):
        result = f"> {self.path}\n| {self.reason}"
        if self.diff and show_diff:
            result += f"\n```\n{self.diff}\n```"
        return result


def get_changed_files(commit_hash):
    """Returns a list of files changed in the given commit."""
    try:
        result = subprocess.run(
            ["git", "diff-tree", "--no-commit-id", "--name-only", "-r", commit_hash],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        files = result.stdout.strip().split("\n")
        return files if files[0] else []
    except subprocess.CalledProcessError as e:
        raise Exception(f"Error running git command: {e}")


def check_c_diff(path):
    # TODO - use 'difflib' (for better win support?)
    #
    # start the first process
    p1 = subprocess.Popen(
        ["uncrustify", "-c", str(rootd.joinpath(Path("uncrustify.cfg"))), "-f", path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    # start the second process, taking input from the first process
    p2 = subprocess.Popen(
        ["diff", path, "-"],
        stdin=p1.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    # close the first process's stdout to allow p1 to receive a SIGPIPE if p2 exits
    p1.stdout.close()

    # capture output and errors
    stdout, stderr = p2.communicate()

    if stdout != "":
        errors.append(
            Error(
                path=path,
                reason="Not correctly uncrustified!",
                diff=stdout,
            )
        )


def check_tga(path):
    # NOTE - unused, as 2.60b actually supports RLE TGA
    # def check_rle(path):
    #     # run ImageMagick's identify command to get metadata
    #     result = subprocess.run(
    #         ["identify", "-verbose", path],
    #         capture_output=True,
    #         text=True,
    #         check=True,
    #     )

    #     # check if 'RLE' appears in the output
    #     if "RLE" in result.stdout:
    #         errors.append(
    #             Error(
    #                 path=path,
    #                 reason="Detected an RLE-encoded TGA file, which is not supported by Vanilla ET.",
    #             )
    #         )

    def check_origin(path):
        with open(path, "rb") as file:
            file.seek(16)  # Move to the 17th byte (zero-based index)
            byte = file.read(1)  # Read one byte
            if byte != b"\x20":
                errors.append(
                    Error(
                        path=path,
                        reason="TGA file does not have bottom-left field order, but top-left, which is not supported by Vanilla ET.",
                    )
                )

    check_origin(path)


def check_jpg(path):
    with open(path, "rb") as f:
        data = f.read()

    # Ensure the file starts with a JPEG SOI marker (0xFFD8)
    if not data.startswith(b"\xff\xd8"):
        errors.append(
            Error(
                path=path,
                reason="Not a valid JPEG file",
            )
        )
        return

    # Scan for Start of Scan (SOS) markers (0xFFDA)
    sos_count = data.count(b"\xff\xda")

    # Progressive JPEGs have multiple SOS markers
    if sos_count > 1:
        errors.append(
            Error(
                path=path,
                reason="JPEG file seems to be progressive, which is not supported by Vanilla ET. ",
            )
        )


def check_commit(commit_hash):
    errors.clear()
    changed_files = get_changed_files(commit_hash)

    # check files
    for changed_file in changed_files:
        path = Path(changed_file)

        if not path.exists():
            print("? skipping missing: " + str(path))
            continue

        abspath = str(rootd.joinpath(path).absolute())
        print("* " + str(path))
        match path.suffix.lower():
            case ".tga":
                check_tga(abspath)
            case ".jpg" | ".jpeg":
                check_jpg(abspath)
            case ".c" | ".h":
                check_c_diff(abspath)

    # summarize errors
    if len(errors) > 0:
        print("------")
        for error in errors:
            sys.stderr.write(str(error) + "\n")

        return len(errors)

    return 0


def main(args):
    if args.diff:
        global show_diff
        show_diff = True

    commits = []

    # assemble commits
    if not args.commit_hash:
        commits.append(
            subprocess.check_output(["git", "rev-parse", "HEAD"]).decode().strip()
        )
    else:
        import io

        lines = io.StringIO(
            subprocess.check_output(
                ["git", "rev-list", "..." + args.commit_hash]
            ).decode()
        )
        for line in lines:
            line = line.strip()
            commits.append(line)

    # check commits
    error_count = 0
    for i, commit in enumerate(commits):
        if i > 0:
            print("======")

        error_count += check_commit(commit)

    if error_count > 0:
        print(f"\nDetected {error_count} errors !")
        sys.exit(1)


def cli():
    import argparse

    parser = argparse.ArgumentParser(
        description="Check files between git commits for CI purposes."
    )

    parser.add_argument(
        "commit_hash",
        type=str,
        nargs="?",
    )

    parser.add_argument(
        "-d",
        "--diff",
        dest="diff",
        action="store_true",
        help="Show diff of errors",
    )

    args = parser.parse_args(sys.argv[1:])

    main(args)


if __name__ == "__main__":
    cli()
