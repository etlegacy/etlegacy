#!/usr/bin/env python3
"""
fetch-single-file-from-git-repo.py

Clone a git repository into a temporary directory (minimally), print one file to
stdout, then delete the repo again.

Examples:
  python fetch-single-file-from-git-repo.py https://github.com/user/repo.git path/to/file.txt
  python fetch-single-file-from-git-repo.py git@github.com:user/repo.git path/to/file.txt --ref main
  python fetch-single-file-from-git-repo.py git@github.com:user/repo.git path/to/file.bin --ref v1.2.3 > file.bin
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import PurePosixPath


def die(msg: str, code: int = 1) -> None:
    print(msg, file=sys.stderr)
    raise SystemExit(code)


def safe_git_path(p: str) -> str:
    # Git paths in rev:path use forward slashes.
    p = p.replace("\\", "/").lstrip("/")
    parts = PurePosixPath(p).parts
    if not p or any(part in ("..", "") for part in parts):
        die(f"Invalid file path: {p!r} (must be a relative path inside the repo)")
    return p


def run_git(args: list[str], cwd: str | None = None, quiet: bool = False) -> None:
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")  # avoid hanging on credential prompts

    stdout = subprocess.DEVNULL if quiet else None
    proc = subprocess.run(
        ["git", *args],
        cwd=cwd,
        env=env,
        stdout=stdout,
        stderr=subprocess.PIPE,
        check=False,
    )
    if proc.returncode != 0:
        err = proc.stderr.decode(errors="replace").strip()
        die(f"git {' '.join(args)} failed (exit {proc.returncode}):\n{err}")


def try_clone(repo: str, dest: str) -> None:
    # Minimal-ish: shallow + no checkout + (if supported) partial clone without blobs.
    base = ["clone", "--depth=1", "--no-checkout", "--single-branch", repo, dest]

    # Prefer partial clone to avoid downloading blobs for the whole repo.
    preferred = [
        "clone",
        "--filter=blob:none",
        "--depth=1",
        "--no-checkout",
        "--single-branch",
        repo,
        dest,
    ]

    # Try preferred first; fall back if the installed git is too old for --filter.
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")
    proc = subprocess.run(
        ["git", *preferred], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE
    )
    if proc.returncode == 0:
        return

    err = proc.stderr.decode(errors="replace")
    # If it failed for some other reason (auth/network), surface that error.
    if (
        "--filter" not in err
        and "filter" not in err
        and "unknown option" not in err.lower()
    ):
        die(f"git clone failed:\n{err.strip()}")

    # Retry without --filter.
    proc2 = subprocess.run(
        ["git", *base], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE
    )
    if proc2.returncode != 0:
        die(f"git clone failed:\n{proc2.stderr.decode(errors='replace').strip()}")


def try_fetch_ref(repo_dir: str, ref: str) -> None:
    # Fetch just the requested ref shallowly; prefer blob-less filter if available.
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")

    preferred = ["fetch", "--depth=1", "--filter=blob:none", "origin", ref]
    base = ["fetch", "--depth=1", "origin", ref]

    proc = subprocess.run(
        ["git", "-C", repo_dir, *preferred],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    if proc.returncode == 0:
        return

    err = proc.stderr.decode(errors="replace")
    if (
        "--filter" not in err
        and "filter" not in err
        and "unknown option" not in err.lower()
    ):
        die(f"git fetch failed:\n{err.strip()}")

    proc2 = subprocess.run(
        ["git", "-C", repo_dir, *base],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    if proc2.returncode != 0:
        die(f"git fetch failed:\n{proc2.stderr.decode(errors='replace').strip()}")


def stream_git_show(repo_dir: str, rev: str, path_in_repo: str) -> None:
    env = os.environ.copy()
    env.setdefault("GIT_TERMINAL_PROMPT", "0")

    spec = f"{rev}:{path_in_repo}"
    proc = subprocess.Popen(
        ["git", "-C", repo_dir, "show", spec],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert proc.stdout is not None

    try:
        for chunk in iter(lambda: proc.stdout.read(1024 * 1024), b""):
            sys.stdout.buffer.write(chunk)
    finally:
        sys.stdout.buffer.flush()

    _, stderr = proc.communicate()
    if proc.returncode != 0:
        die(
            f"git show {spec} failed (exit {proc.returncode}):\n{stderr.decode(errors='replace').strip()}"
        )


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Clone a repo into a temp dir, print one file to stdout, then delete the repo."
    )
    ap.add_argument("repo", help="Repository URL (https://… or git@…:… etc.)")
    ap.add_argument("file", help="Path to file inside the repo (e.g. path/to/file.txt)")
    ap.add_argument(
        "--ref",
        default=None,
        help="Optional branch/tag/commit to read from (default: repo's default HEAD)",
    )
    args = ap.parse_args()

    path_in_repo = safe_git_path(args.file)

    try:
        with tempfile.TemporaryDirectory(prefix="print-repo-file-") as td:
            repo_dir = os.path.join(td, "repo")
            try_clone(args.repo, repo_dir)

            rev = "HEAD"
            if args.ref:
                try_fetch_ref(repo_dir, args.ref)
                rev = "FETCH_HEAD"

            stream_git_show(repo_dir, rev, path_in_repo)
    except FileNotFoundError:
        die("Error: 'git' not found on PATH. Please install Git and try again.", 127)


if __name__ == "__main__":
    main()
