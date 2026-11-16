#!/usr/bin/env bash
#
# mayhem/build.sh — builds the FastNoise2 fuzz harnesses, their -standalone reproducers, and the
# known-answer probe that mayhem/test.sh runs. Runs inside the commit image as `mayhem` in /mayhem,
# offline: FastSIMD (which upstream's CMake would fetch through CPM) is pre-fetched in the Dockerfile.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${FASTSIMD_COMMIT:=f7e9ce6fca475731c0a3c115ec29dcfd36886b2a}"
# cmake picks the compilers up from $CC/$CXX (by name); passing them with -D would make a re-run reset the cache.
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

FASTSIMD_SRC=/opt/toolchains/deps/fastsimd-${FASTSIMD_COMMIT}/src
OUT="$SRC/mayhem/out"
mkdir -p "$OUT"

# 1) Fuzz build: upstream's library (and FastSIMD) with the sanitizers + DWARF 3, then the harnesses.
#    -fsanitize=fuzzer-no-link puts SanCov coverage instrumentation into the library itself.
FUZZ_FLAGS="-O2 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S mayhem -B mayhem/build-fuzz -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE= \
    -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
    -DCPM_FastSIMD_SOURCE="$FASTSIMD_SRC" -DCPM_USE_LOCAL_PACKAGES=OFF \
    -DMAYHEM_FUZZ_LINK_FLAGS="$LIB_FUZZING_ENGINE" \
    -DMAYHEM_LSAN_OFF="$SRC/mayhem/lsan_off.cc" \
    -DMAYHEM_STANDALONE_MAIN="${STANDALONE_FUZZ_MAIN:?STANDALONE_FUZZ_MAIN unset}"
cmake --build mayhem/build-fuzz -j"$MAYHEM_JOBS"

for t in fuzz_encoded_tree fuzz_node_tree_binary fuzz_nodedata fuzz_generate; do
    cp "$(find mayhem/build-fuzz -maxdepth 2 -type f -name "$t" | head -n1)" "$OUT/$t"
    cp "$(find mayhem/build-fuzz -maxdepth 2 -type f -name "$t-standalone" | head -n1)" "$OUT/$t-standalone"
done

# 2) Test build: the same library with the NORMAL flags (no sanitizers), plus the known-answer probe.
TEST_FLAGS="-O2 $COVERAGE_FLAGS"
cmake -S mayhem -B mayhem/build-tests -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE= \
    -DCMAKE_C_FLAGS="$TEST_FLAGS" -DCMAKE_CXX_FLAGS="$TEST_FLAGS" \
    -DCPM_FastSIMD_SOURCE="$FASTSIMD_SRC" -DCPM_USE_LOCAL_PACKAGES=OFF \
    -DMAYHEM_KAT=ON
cmake --build mayhem/build-tests -j"$MAYHEM_JOBS" --target kat
cp "$(find mayhem/build-tests -maxdepth 2 -type f -name kat | head -n1)" "$OUT/kat"
