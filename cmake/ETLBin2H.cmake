# Converts a binary file (e.g. compiled SPIR-V) into a C header containing a
# `static const unsigned char <ARRAY_NAME>[]` byte array plus a matching
# `<ARRAY_NAME>_SIZE` length constant.
#
# Expected to be invoked in script mode:
#   cmake -DIN_FILE=<path> -DOUT_FILE=<path> -DARRAY_NAME=<name> -P ETLBin2H.cmake

if(NOT IN_FILE OR NOT OUT_FILE OR NOT ARRAY_NAME)
	message(FATAL_ERROR "ETLBin2H.cmake requires IN_FILE, OUT_FILE and ARRAY_NAME to be set")
endif()

file(READ "${IN_FILE}" HEX_CONTENT HEX)

string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")

string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," HEX_BYTES "${HEX_CONTENT}")

file(WRITE "${OUT_FILE}"
	"// Auto-generated from ${IN_FILE} by ETLBin2H.cmake - do not edit\n"
	"#pragma once\n"
	"static const unsigned char ${ARRAY_NAME}[] = {\n${HEX_BYTES}\n};\n"
	"static const unsigned int ${ARRAY_NAME}_SIZE = ${BYTE_COUNT}u;\n"
)
