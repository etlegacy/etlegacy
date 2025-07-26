#!/usr/bin/env python3
# Collects and checks GitHub Build Logs
# (used by CI)
import fnmatch
import os
import re
import requests
import sys


EXPLICITLY_IGNORED_WARNINGS = {
    "android": [],
    "lnx-aarch64": [
        # see https://github.com/etlegacy/etlegacy/issues/3102
        # [
        #     "../src/qcommon/q_shared.c",
        #     "warning: 'strncpy' specified bound depends on the length of the source argument [-Wstringop-overflow=]",
        # ],
    ],
    "lnx32": [
        # [
        #     "/code/src/cgame/cg_ents.c",
        #     "warning: variable 'version' set but not used [-Wunused-but-set-variable]",
        # ],
    ],
    "lnx64": [],
    "osx": [],
    "win": [],
    "win64": [],
}

# Regex to match ANSI escape codes (e.g., color sequences)
ansi_escape = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")


def wrangle_os_paths(arch: str, lst: list[str]):
    if arch.startswith("win"):
        return [x.replace("/", "\\") for x in lst]
    else:
        return lst


def clean_and_process_file(arch: str, filename: str):
    pertinent_lines = []
    skipped_lines = []

    # different archs report warnings differently (:
    stanza = "warning:"
    if arch.startswith("win"):
        stanza = ": warning C"

    with open(filename, "r", encoding="utf-8", errors="replace") as f:
        for raw_line in f:
            # Normalize Windows CRLF to LF
            line = raw_line.rstrip("\r\n") + "\n"
            # Remove ANSI escape codes
            line = ansi_escape.sub("", line)

            if not stanza in line:
                continue

            # Handle only game code
            if not any(
                sub in line
                for sub in wrangle_os_paths(
                    arch,
                    [
                        "src/tinygettext",
                    ],
                )
            ) and any(
                sub in line
                for sub in wrangle_os_paths(
                    arch,
                    [
                        "etlegacy/src/",
                        "src/botlib/",
                        "src/cgame/",
                        "src/client/",
                        "src/db/",
                        "src/game/",
                        "src/irc/",
                        "src/luacjson/",
                        "src/luasql/",
                        "src/null/",
                        "src/Omnibot/",
                        "src/qcommon/",
                        "src/renderer/",
                        "src/renderer2/",
                        "src/renderer_vk/",
                        "src/renderercommon/",
                        "src/rendererGLES/",
                        "src/sdl/",
                        "src/server/",
                        "src/sys/",
                        "src/tests/",
                        "src/tinygettext/",
                        "src/tools/",
                        "src/tvgame/",
                        "src/ui/",
                    ],
                )
            ):
                ## skip explicitly ignored warnings - TODO fixup for
                ## Windows
                # strip timestamp
                parts = line.split(sep=" ", maxsplit=1)[1]
                # split at 'warning:'
                parts = parts.split(sep=stanza, maxsplit=1)
                # reattach 'warning:' to rhs
                msg = stanza + parts[1]
                msg = msg.strip()
                # strip the line/col info
                fpath = parts[0].rsplit(sep=":", maxsplit=3)[0]
                fpath = fpath.strip()

                ignored = False
                for ignore_fpath, ignore_msg in EXPLICITLY_IGNORED_WARNINGS[arch]:
                    if fpath == ignore_fpath and msg == ignore_msg:
                        skipped_lines.append("EXPLICIT: " + line)
                        ignored = True
                        break

                if ignored:
                    continue

                pertinent_lines.append(line)
                continue

            skipped_lines.append(line)
            continue

    return [pertinent_lines, skipped_lines]


def find_txt_files(root_dir):
    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in fnmatch.filter(filenames, "job-logs-*.txt"):
            yield os.path.join(dirpath, filename)


def check_logs(args):
    # global enforce
    failed = 0
    skipped = 0

    found_logs_count = 0

    root_directory = "."  # change to your target directory
    for txt_file in find_txt_files(root_directory):
        found_logs_count += 1
        arch = (txt_file.split("job-logs-")[1]).split(".txt")[0]

        is_non_pertinent = "android" in txt_file or "win64" in txt_file
        pertinent_lines, skipped_lines = clean_and_process_file(arch, txt_file)

        if not args.show_skipped and len(pertinent_lines) <= 0:
            continue

        # print("###", f"{arch:11}", txt_file, "- START {{{")
        print("###", f"{arch:11} - START", f"({txt_file})", "{{{")
        if is_non_pertinent:
            print(
                f"# INFO - {arch.capitalize()} warnings are never pertinent for now ..."
            )
        else:
            failed += len(pertinent_lines)

        for line in pertinent_lines:
            print(line, end="")
            if args.github:
                # strip timestamp
                line = line.split(sep=" ", maxsplit=1)[1].strip()
                if is_non_pertinent:
                    print(f"::warning ::{arch} - {line}")
                else:
                    print(f"::error ::{arch} - {line}")

        skipped += len(skipped_lines)
        if args.show_skipped:
            print("")
            print("## SKIPPED LINES:")
            for line in skipped_lines:
                print(line, end="")

        print("###", f"{arch:11} - END  ", f"({txt_file})", "}}}")

    print("")
    print(
        f"{skipped} warnings were skipped/excluded (assumed to not be directly from ETL, but deps)..."
    )
    if failed > 0:
        print(
            f"{failed} pertinent warnings were emitted (ignoring Android), failing..."
        )
        exit(1)
    else:
        print(f"No pertinent warnings were found.")

    assert found_logs_count == len(
        EXPLICITLY_IGNORED_WARNINGS
    ), "Did not find all the necessary arch logs."


def collect_logs():
    # all archs
    needs_list = EXPLICITLY_IGNORED_WARNINGS.keys()

    # Get required environment variables
    repo = os.environ.get("GITHUB_REPOSITORY") or "etlegacy/etlegacy"
    run_id = os.environ.get("GITHUB_RUN_ID")
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")

    if not (run_id and token):
        print("Missing GITHUB_RUN_ID, or token (GH_TOKEN or GITHUB_TOKEN)!")
        sys.exit(1)

    api = "https://api.github.com"
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github.v3+json",
    }

    # List jobs for this workflow run
    jobs_url = f"{api}/repos/{repo}/actions/runs/{run_id}/jobs"
    resp = requests.get(jobs_url, headers=headers)
    if resp.status_code != 200:
        print(f"Failed to fetch jobs: {resp.status_code} {resp.text}")
        sys.exit(1)
    jobs = resp.json().get("jobs", [])

    for need in needs_list:
        # Find job with matching name
        job = next((j for j in jobs if j["name"] == need), None)
        if job:
            job_id = job["id"]
            log_url = f"{api}/repos/{repo}/actions/jobs/{job_id}/logs"
            log_resp = requests.get(log_url, headers=headers)
            if log_resp.status_code == 200:
                with open(f"job-logs-{need}.txt", "wb") as f:
                    f.write(log_resp.content)
                print(f"Downloaded log for '{need}' as job-logs-{need}.txt")
            else:
                print(f"Failed to download log for '{need}': {log_resp.status_code}")
        else:
            print(f"Job '{need}' not found or did not run.")

    print("")


def cli():
    import argparse

    parser = argparse.ArgumentParser(
        description="Analyze build logs of various platforms. Errors if pertinent ones were found."
    )

    parser.add_argument(
        "-ss",
        "--show-skipped",
        dest="show_skipped",
        action="store_true",
        help="Show skipped lines (for debugging purposes)",
    )

    parser.add_argument(
        "-scl",
        "--skip-collecting-logs",
        dest="skip_collecting_logs",
        action="store_true",
        help="Skip collecting logs from GitHub Actions",
    )

    parser.add_argument(
        "-gh",
        "--github",
        action="store_true",
        help="Print special messages that Github Actions integrates better for error reporting",
    )

    args = parser.parse_args(sys.argv[1:])

    if not args.skip_collecting_logs:
        collect_logs()

    check_logs(args)


if __name__ == "__main__":
    cli()
