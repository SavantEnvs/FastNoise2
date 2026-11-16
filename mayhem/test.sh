#!/usr/bin/env bash
#
# mayhem/test.sh — runs the known-answer probe (mayhem/kat/kat.cpp, built by build.sh with the
# project's normal flags) and reports CTRF counts. The probe drives FastNoise2's real public API on the
# SSE2 SIMD path (single-sample noise values, decoded node trees, Base64, the node metadata
# round trip) and compares every result against the recorded values in mayhem/kat/expected.txt.
# Upstream ships no tests of its own. A no-op / early-exit "fix" leaves no PASS lines and fails here.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

KAT="$SRC/mayhem/out/kat"
if [ ! -x "$KAT" ]; then
  echo "missing $KAT — mayhem/build.sh should have built it" >&2
  emit_ctrf "fastnoise2-kat" 0 1 0
  exit 1
fi

out="$("$KAT" "$SRC/mayhem/kat/expected.txt" 2>&1)"
rc=$?
printf '%s\n' "$out"

passed=$(printf '%s\n' "$out" | grep -c '^KAT PASS ')
failed=$(printf '%s\n' "$out" | grep -c '^KAT FAIL ')
done_n=$(printf '%s\n' "$out" | sed -n 's/^KAT DONE \([0-9]*\)$/\1/p')
want=$(grep -c . "$SRC/mayhem/kat/expected.txt")

# The probe must have run to completion: a DONE line covering at least every recorded value.
if [ "$rc" -ne 0 ] || [ -z "$done_n" ] || [ "$done_n" -lt "$want" ] || [ $((passed + failed)) -ne "$done_n" ]; then
  failed=$((failed + 1))
fi
emit_ctrf "fastnoise2-kat" "$passed" "$failed" 0
