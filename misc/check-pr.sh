#!/bin/bash

# Redirect output to stderr.
exec 1>&2

RET=0

changed_files=$(git diff --name-only master)
crustified_files=""

for file in $changed_files;
do
	# Only do checkups on c,h,cpp and hpp files
	if [[ $file != *.c ]] && [[ $file != *.h ]] && [[ $file != *.cpp ]] && [[ $file != *.hpp ]]; then
		echo "Skipping: $file"
		continue
	fi
	echo "Checking file: $file"
	# compare uncrustified file with original
	uncrustify -q -c uncrustify.cfg -f $file | cmp --quiet $file -
	if [[ $? = 1 ]]; then
		crustified_files+=" $file"
		RET=1
	fi
done

if [[ $RET = 1 ]];
then
	echo "Formatting incorrect on some file(s)"
	echo "Please run 'uncrustify --no-backup -c uncrustify.cfg $crustified_files' and update the pull request."
fi

exit $RET

