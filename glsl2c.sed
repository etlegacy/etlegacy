# sed script to convert a text file to a C string
# remove comment blocks
/\/\*/{
	# here we've got an /*, append lines until get the corresponding
	# */
	:x
	/\*\//!{
		N
		bx
	}
	# delete /*...*/
	s/\/\*.*\*\///
}

#1 iconst char *charName =	# the name of the string
s/\x1b\[.\{1,5\}m//g		# remove possible control chars
/^M/d						# remove carriage return
s/\r//						# remove carriage return
/^[ \t]*$/d					# remove empty lines
/^[ \t]*\/\/.*$/d			# remove commented out lines
s/[ \t]*\/\/.*$//g			# remove normal comments
s/\\/\\\\/g					# escapes backslashes
s/"/\\"/g					# escapes quotes
s/\t/\\t/g					# converts tabs to \t
s/  /\\t/g					# converts tabs to \t
s/^/"/						# adds quotation mark to the beginning of a line
s/$/\\n"/					# adds \n and a quotation mark to the end of a line
$s/$/;/						# add ; to the last line to finnish the variable value