#!/bin/sh
$CC "$@" -static-libgcc "$($CC --print-file-name='libstdc++.a')" -lm
