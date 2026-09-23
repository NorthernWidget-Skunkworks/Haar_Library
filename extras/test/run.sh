#!/bin/sh
# Compile Haar.cpp on the host against the stubs here and diff the output
# against baseline.txt. Usage: ./run.sh            (test)
#                             ./run.sh --record   (rewrite baseline.txt)
cd "$(dirname "$0")" || exit 1
NW_CORE="${NW_CORE:-../../../NW_Core}"   # NW_Core checkout: headers (and later sources) the library depends on
g++ -std=c++17 -Wall -Wno-unused-function -I"$NW_CORE/extras/test" -I"$NW_CORE/src" -o haar_test test_output.cpp "$NW_CORE/src/NW_Device.cpp" || exit 1
./haar_test > output.txt || exit 1
if [ "$1" = "--record" ]; then cp output.txt baseline.txt; echo "baseline recorded"; exit 0; fi
if diff -u baseline.txt output.txt; then echo "OK: output identical to baseline"; else echo "FAIL: output differs from baseline"; exit 1; fi
