#!/bin/sh
# Host-side harness: compile src/Haar.cpp against the NW_Core stubs and diff
# the output against baseline.txt. Usage: ./run.sh [--record]
cd "$(dirname "$0")" || exit 1
NW_CORE="${NW_CORE:-../../../NW_Core}"
exec "$NW_CORE/extras/test/run_library.sh" haar_test "$1"
