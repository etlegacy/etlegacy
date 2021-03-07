#!/usr/bin/env bash
# encoding: utf-8

# Handle release process
# Updates the VERSION.txt file and creates the tag

set -Eeuo pipefail

# current Git branch
branch=$(git symbolic-ref HEAD | sed -e 's,.*/\(.*\),\1,')

# Require master
if [ ! "$branch" = "master" ]; then
	echo "Not in master exiting"
	exit 1
fi

# Require the current working directory to be clean
if [ ! -z "$(git status --porcelain)" ]; then
	echo "Git repository is not clean exiting"
	exit 1
fi

# Parse the version file
major=$(grep "VERSION_MAJOR" VERSION.txt | cut -d" " -f2)
minor=$(grep "VERSION_MINOR" VERSION.txt | cut -d" " -f2)
patch=$(grep "VERSION_PATCH" VERSION.txt | cut -d" " -f2)
version_changed=
version_message=

parse_params() {
	while :; do
		case "${1-}" in
		-v | --verbose) set -x ;;
		--major)
			major=$((major+1))
			minor=0
			patch=0
			version_changed=true
			;;
		--minor)
			minor=$((minor+1))
			patch=0
			version_changed=true
			;;
		--patch)
			patch=$((patch+1))
			version_changed=true
			;;
		-m | --message)
			version_message="${2-}"
			shift
			;;
		-?*) die "Unknown option: $1" ;;
		*) break ;;
		esac
		shift
	done

	args=("$@")
	return 0
}

parse_params "$@"

# If nothing has changed then just exit
if [ -z $version_changed ]; then
	echo "Nothing to do"
	exit 0
fi

# Sorry tag is already taken.
if [ $(git tag -l "v$major.$minor.$patch") ]; then
	echo "Tag 'v$major.$minor.$patch' was taken"
	exit 1
fi

if [ $patch = 0 ] &&  [ $(git tag -l "v$major.$minor") ]; then
	echo "Tag 'v$major.$minor' was taken"
	exit 1
fi

if [ -z $version_message ]; then
	version_message="Version $major.$minor.$patch"
fi

echo "Ready to commit and tag a new version: $major.$minor.$patch"
echo "Version message will be: $version_message"
read -p "Ready to commit? [Y/N]: " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
	# Update the version file.
	perl -pi -e "s/(VERSION_MAJOR)\s+[0-9]+/\1 $major/g" VERSION.txt
	perl -pi -e "s/(VERSION_MINOR)\s+[0-9]+/\1 $minor/g" VERSION.txt
	perl -pi -e "s/(VERSION_PATCH)\s+[0-9]+/\1 $patch/g" VERSION.txt

	# Create the release commit
	git commit -am "Incrementing version number to $major.$minor.$patch"
	# Tag it like a champ!
	git tag -a "v$major.$minor.$patch" -m "$version_message"

	echo "Committed and tagged a new release"
	read -p "Push commit and tag to remote? [Y/N]: " -n 1 -r
	echo
	if [[ $REPLY =~ ^[Yy]$ ]]
	then
		git push origin master
		git push origin "v$major.$minor.$patch"
		echo "Pushed data to remote. Congrats!"
	else
		echo "You need to 'git push origin master' and 'git push origin --tags' manually."
	fi
else
  echo "Chicken!"
fi
