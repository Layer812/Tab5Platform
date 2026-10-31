#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/components/tab5_platform/include" \
  "$ROOT/components/tab5_platform/src/tab5_geometry.c" \
  "$ROOT/tests/test_geometry.c" -o "$TMP/geometry"
"$TMP/geometry"
cc -std=c11 -Wall -Wextra -Werror \
  -I"$ROOT/tests/stubs" \
  -I"$ROOT/sample/mame/include" \
  -I"$ROOT/components/tab5_platform/include" \
  "$ROOT/sample/mame/src/t5_sample_mame.c" \
  "$ROOT/tests/test_mame_stub.c" -o "$TMP/mame_stub"
"$TMP/mame_stub"
