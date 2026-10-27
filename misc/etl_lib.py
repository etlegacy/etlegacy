import locale
import os
import sys
import subprocess
import difflib
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional

ROOT_DIR = Path(__file__).resolve().parent.parent
CWD = Path.cwd()

# Directories (relative to the repo root) containing third-party code that
# is never formatted or checked by our tooling.
EXCLUDED_PATHS = [
    "libs/",
    "vendor/",
]

# The formatter handling each (lower-case) file extension. Shared so that
# autoformat and check-changes always cover the same set of files.
#
# NOTE: .glsl files are deliberately NOT listed, i.e. they are skipped by both
# autoformat and check-changes. Do not add them here without agreeing on
# formatting them first.
FORMATTER_BY_EXT = {
    ".c": "uncrustify",
    ".cpp": "uncrustify",
    ".h": "uncrustify",
    ".py": "black",
    ".sh": "shfmt",
    ".yml": "prettier",
    ".yaml": "prettier",
}


def decode_subprocess_output(data: bytes) -> str:
    # Most tool output in this repo is UTF-8, but Windows helper text can still
    # arrive in the active locale encoding.
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return data.decode(
            locale.getpreferredencoding(False) or "utf-8", errors="replace"
        )


def run_command(cmd: List[str], check=True) -> subprocess.CompletedProcess:
    raw_result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    result = subprocess.CompletedProcess(
        raw_result.args,
        raw_result.returncode,
        decode_subprocess_output(raw_result.stdout or b""),
        decode_subprocess_output(raw_result.stderr or b""),
    )
    if check and result.returncode:
        msg = f"$ {" ".join([str(x) for x in cmd])}\nreturncode: {result.returncode}"
        if result.stdout:
            msg += "\nstdout:\n" + result.stdout
        if result.stderr:
            msg += "\nstderr:\n" + result.stderr
        raise Exception(msg)
    return result


def run_git_command(cmd: List[str]) -> List[str]:
    result = run_command(cmd, check=True)
    return result.stdout.strip().splitlines()


def get_changed_files_for_commit(commit_hash: str) -> List[Path]:
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


def get_current_git_branch(path="."):
    return run_git_command(["git", "-C", path, "rev-parse", "--abbrev-ref", "HEAD"])[0]


def get_commits(base_commit: Optional[str]) -> List[str]:
    if base_commit:
        return run_git_command(["git", "rev-list", f"...{base_commit}"])
    return [run_git_command(["git", "rev-parse", "HEAD"])[0]]


def get_changed_files(commit_hash: str) -> set[str]:
    print("Considering changes towards", f"'{commit_hash}' ...")

    changed_files = set()

    staged_unstaged_files = get_staged_unstaged_files()
    if staged_unstaged_files:
        print(" picking up staged/unstaged files")
        changed_files.update(staged_unstaged_files)

    for i, commit in enumerate(get_commits(commit_hash)):
        changed_files.update(get_changed_files_for_commit(commit))

    return changed_files


def get_all_tracked_files() -> set[Path]:
    """Returns all files checked into git (as absolute paths)."""
    return {
        ROOT_DIR / path
        for path in run_git_command(["git", "-C", str(ROOT_DIR), "ls-files"])
    }


def get_formatter(path: Path) -> Optional[str]:
    """Returns the name of the formatter responsible for the file (if any)."""
    return FORMATTER_BY_EXT.get(Path(path).suffix.lower())


def is_excluded_path(path: Path) -> bool:
    """Returns whether the path is below one of the EXCLUDED_PATHS."""
    path = Path(path).resolve()
    return any(path.is_relative_to((ROOT_DIR / p).resolve()) for p in EXCLUDED_PATHS)


def argparse_add_file_selection_params(parser):
    import argparse

    # optionally read default upstream_remote_branch from
    # 'etlegacy/.check-changes-master-branch'
    upstream_remote_branch = "origin/master"
    upstream_remote_branch_path = ROOT_DIR / ".upstream-remote-branch"
    if upstream_remote_branch_path.exists():
        with open(upstream_remote_branch_path, "r", encoding="utf-8") as f:
            upstream_remote_branch = f.read().strip()

    parser.add_argument(
        "commit_hash",
        default=upstream_remote_branch,
        nargs="?",
        help="Compare with this commit (defaults to HEAD)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Consider all git-tracked files from the repo root instead of only the changes.",
    )


def get_selected_files(args) -> set[Path]:
    """
    Returns the files selected via argparse_add_file_selection_params():
    all git-tracked files with --all, otherwise the changed ones.
    """
    if args.all:
        print("Considering all git-tracked files ...")
        return get_all_tracked_files()
    return get_changed_files(args.commit_hash)


def rel(path: Path) -> Path:
    return path.relative_to(ROOT_DIR)


def rel_str(path: Path) -> str:
    return str(rel(path))
