import os
import sys
import subprocess
import difflib
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional

ROOT_DIR = Path(__file__).resolve().parent.parent
CWD = Path.cwd()


def run_command(cmd: List[str], check=True) -> subprocess.CompletedProcess:
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
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
    result = subprocess.run(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True
    )
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


def argparse_add_commit_param(parser):
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


def rel(path: Path) -> Path:
    return path.relative_to(ROOT_DIR)


def rel_str(path: Path) -> str:
    return str(rel(path))
