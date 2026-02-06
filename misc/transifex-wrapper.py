#!/usr/bin/env python3
# transifex-wrapper
#
# Pure-stdlib wrapper that:
#  1) picks the correct pinned Transifex CLI asset URL for this OS/arch (v1.6.17)
#  2) downloads + unpacks it (tar.gz or zip)
#  3) installs it to: .transifex-wrapper/tx{.exe}
#  4) runs it, forwarding CLI args
#
# IMPORTANT behavior per requirements:
# - If installed for the exact URL already (downloaded-url.txt matches), DO NOT re-download
#   and DO NOT do conditional GETs. URL change is required to re-download.
# - Stores only the installed URL in: .transifex-wrapper/downloaded-url.txt (single line).

from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Tuple

VERSION = "v1.6.17"
BASE = f"https://github.com/transifex/cli/releases/download/{VERSION}"

# Hard-coded allowlist from the release page (must match exactly).
ALLOWED_URLS = {
    f"{BASE}/tx-darwin-amd64.tar.gz",
    f"{BASE}/tx-darwin-arm64.tar.gz",
    f"{BASE}/tx-linux-386.tar.gz",
    f"{BASE}/tx-linux-amd64.tar.gz",
    f"{BASE}/tx-linux-arm64.tar.gz",
    f"{BASE}/tx-windows-386.zip",
    f"{BASE}/tx-windows-amd64.zip",
}


def is_windows() -> bool:
    return os.name == "nt" or sys.platform.lower().startswith("win")


def detect_os_arch() -> Tuple[str, str]:
    """
    Returns (os_id, arch_id) where:
      os_id  in {"linux","darwin","windows"}
      arch_id in {"amd64","arm64","386"}
    """
    sp = sys.platform.lower()
    if sp.startswith("linux"):
        os_id = "linux"
    elif sp.startswith("darwin"):
        os_id = "darwin"
    elif sp.startswith("win"):
        os_id = "windows"
    else:
        raise RuntimeError(f"Unsupported OS: sys.platform={sys.platform!r}")

    m = (platform.machine() or "").lower()
    if m in ("x86_64", "amd64"):
        arch_id = "amd64"
    elif m in ("aarch64", "arm64"):
        arch_id = "arm64"
    elif m in ("i386", "i686", "x86"):
        arch_id = "386"
    else:
        raise RuntimeError(
            f"Unsupported architecture for this pinned release: {m!r} (need arm64/amd64/386)"
        )

    return os_id, arch_id


def choose_url() -> str:
    os_id, arch_id = detect_os_arch()
    ext = "zip" if os_id == "windows" else "tar.gz"
    url = f"{BASE}/tx-{os_id}-{arch_id}.{ext}"
    if url not in ALLOWED_URLS:
        raise RuntimeError(
            "No pinned asset matches detected OS/arch.\n"
            f"Detected: os={os_id!r} arch={arch_id!r}\n"
            f"Interpolated: {url}\n"
            "Allowed URLs:\n  - " + "\n  - ".join(sorted(ALLOWED_URLS))
        )
    return url


def read_downloaded_url(path: Path) -> str | None:
    try:
        # single line, tolerate trailing newline
        return path.read_text(encoding="utf-8").splitlines()[0].strip()
    except FileNotFoundError:
        return None
    except Exception:
        return None


def write_downloaded_url(path: Path, url: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(".tmp")
    tmp.write_text(url.rstrip("\n") + "\n", encoding="utf-8")
    os.replace(tmp, path)


def download_file(url: str, dest: Path, timeout: int = 120) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_suffix(dest.suffix + ".part")
    req = urllib.request.Request(
        url, headers={"User-Agent": "transifex-wrapper/1 (stdlib urllib)"}
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp, tmp.open("wb") as f:
        shutil.copyfileobj(resp, f, length=1024 * 1024)
    os.replace(tmp, dest)


def safe_extract_tar(tf: tarfile.TarFile, dest_dir: Path) -> None:
    # prevent path traversal
    dest_dir = dest_dir.resolve()
    for m in tf.getmembers():
        target = (dest_dir / m.name).resolve()
        if target != dest_dir and not str(target).startswith(str(dest_dir) + os.sep):
            raise RuntimeError(
                f"Refusing to extract tar member outside destination: {m.name!r}"
            )
    tf.extractall(path=dest_dir)


def safe_extract_zip(zf: zipfile.ZipFile, dest_dir: Path) -> None:
    # prevent path traversal
    dest_dir = dest_dir.resolve()
    for name in zf.namelist():
        target = (dest_dir / name).resolve()
        if target != dest_dir and not str(target).startswith(str(dest_dir) + os.sep):
            raise RuntimeError(
                f"Refusing to extract zip member outside destination: {name!r}"
            )
    zf.extractall(path=dest_dir)


def unpack_archive(archive_path: Path, dest_dir: Path) -> None:
    n = archive_path.name.lower()
    if n.endswith(".tar.gz") or n.endswith(".tgz"):
        with tarfile.open(archive_path, "r:gz") as tf:
            safe_extract_tar(tf, dest_dir)
    elif n.endswith(".zip"):
        with zipfile.ZipFile(archive_path, "r") as zf:
            safe_extract_zip(zf, dest_dir)
    else:
        raise RuntimeError(f"Unknown archive type: {archive_path.name}")


def find_tx_binary(extracted_dir: Path) -> Path:
    # Prefer platform-native name, but accept the other as fallback.
    primary = "tx.exe" if is_windows() else "tx"
    fallback = "tx" if is_windows() else "tx.exe"

    def scan(want: str) -> list[Path]:
        return [
            p
            for p in extracted_dir.rglob("*")
            if p.is_file() and p.name.lower() == want.lower()
        ]

    candidates = scan(primary) or scan(fallback)
    if not candidates:
        raise RuntimeError(
            f"Could not locate tx binary inside extracted archive at {extracted_dir}"
        )

    # Prefer shallowest path
    candidates.sort(key=lambda p: (len(p.parts), str(p)))
    return candidates[0]


def make_executable(path: Path) -> None:
    if is_windows():
        return
    st = path.stat()
    path.chmod(st.st_mode | 0o100)  # ensure user-exec at least


def install_for_url(url: str, wrapper_dir: Path, tx_path: Path, url_txt: Path) -> None:
    wrapper_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="transifex-wrapper-") as td:
        tdir = Path(td)
        archive_path = tdir / os.path.basename(url)

        download_file(url, archive_path)
        unpack_dir = tdir / "unpacked"
        unpack_dir.mkdir(parents=True, exist_ok=True)
        unpack_archive(archive_path, unpack_dir)

        tx_src = find_tx_binary(unpack_dir)

        tmp = tx_path.with_suffix(tx_path.suffix + ".new")
        tx_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(tx_src, tmp)
        make_executable(tmp)
        os.replace(tmp, tx_path)

        write_downloaded_url(url_txt, url)


def main() -> int:
    url = choose_url()

    # cd to parent dir of current script
    os.chdir(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

    wrapper_dir = Path.cwd() / ".transifex-wrapper"
    tx_path = wrapper_dir / ("tx.exe" if is_windows() else "tx")
    url_txt = wrapper_dir / "downloaded-url.txt"

    installed_url = read_downloaded_url(url_txt)

    # Only URL change triggers re-download/reinstall.
    if not (tx_path.exists() and installed_url == url):
        try:
            install_for_url(url, wrapper_dir, tx_path, url_txt)
        except Exception as e:
            # If install fails and we don't have a usable existing install for this URL, hard fail.
            if not (tx_path.exists() and installed_url == url):
                print(f"transifex-wrapper: install failed: {e}", file=sys.stderr)
                return 2
            # Otherwise fall back to existing binary.

    cmd = [str(tx_path), *sys.argv[1:]]
    try:
        r = subprocess.run(cmd)
        return int(r.returncode)
    except FileNotFoundError:
        print(f"transifex-wrapper: failed to execute: {tx_path}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
