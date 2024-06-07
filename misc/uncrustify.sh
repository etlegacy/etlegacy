#!/usr/bin/env bash
# uncrustify files in the worktree, either staged ones if no rev is passed, or
# the ones changed in the passed rev
#
# REQUIRES: uncrustify
set -eo pipefail

# cd to parent dir of current script
cd "$(dirname "${BASH_SOURCE[0]}")"
cd ..


is_available() { command -v "$1" &>/dev/null ;}

if ! is_available "uncrustify"; then
	echo "ERROR: Install 'uncrustify' first." >&2
	exit 1
fi

(
	if [[ -z "$1" ]]; then  # default, if no args are passed go through staged files
		git diff --name-only --cached
	else                    # otherwise go through the files changed in the passed rev
		git diff --name-only "$1"
	fi
) | (
	while read -r path; do
		case "$path" in
			src/Omnibot/*                  |\
			src/game/g_etbot_interface.cpp |\
			src/qcommon/crypto/sha-1/*		\
			)
				continue
				;;

			*.c    |\
			*.cpp  |\
			*.glsl |\
			*.h     \
			)
				echo "$path"
				;;
		esac
	done
) | {
	if is_available "parallel"; then  # run via GNU parallel if available
		cpu_core_count=$(nproc --all 2>/dev/null || true )
		cpu_core_count=${cpu_core_count:-4}

		parallel -j"${cpu_core_count}" uncrustify -c uncrustify.cfg -f "{1}" -o /dev/stdout '|' diff -u "{1}" /dev/stdin
	else							  # otherwise sequentially
		while read -r path; do
			{
				echo "> $path"
				uncrustify -c uncrustify.cfg -f "$path" -o /dev/stdout | diff -u "$path" /dev/stdin
			} < /dev/tty
		done
	fi
}
