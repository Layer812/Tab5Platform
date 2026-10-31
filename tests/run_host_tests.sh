#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/components/tab5_platform/include" \
  "$ROOT/components/tab5_platform/src/tab5_geometry.c" \
  "$ROOT/tests/test_geometry.c" -o /tmp/tab5platform_geometry_test
/tmp/tab5platform_geometry_test
