# Out-of-tree driver for the Mayhem integration: builds upstream's library with the fuzz (or oracle)
# flags passed in by mayhem/build.sh, then the harnesses (or the known-answer probe) against it.
cmake_minimum_required(VERSION 3.18)
project(FastNoise2Mayhem C CXX)
set(CMAKE_CXX_STANDARD 17)

option(MAYHEM_KAT "Build the known-answer probe (oracle build) instead of the fuzz harnesses" OFF)
set(MAYHEM_FUZZ_LINK_FLAGS "" CACHE STRING "Fuzzing engine link flags (space separated)")
set(MAYHEM_LSAN_OFF "" CACHE FILEPATH "LSan off-switch hook (lsan_off.cc), linked into every fuzz binary")
set(MAYHEM_STANDALONE_MAIN "" CACHE FILEPATH "LLVM run-once driver (C file) for the -standalone reproducers")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. fastnoise2)

# Metadata.cpp reads multi-byte members out of the decoded byte stream through misaligned
# reinterpret_casts (GetFromDataStream). That is benign on x86 but halts UBSan on nearly every input
# that reaches a member, which would stop the decoders being fuzzed past their first bytes. Relax only
# the alignment check, only for that one file; ASan and the rest of UBSan stay halting.
set_source_files_properties(${FastNoise2_SOURCE_DIR}/src/FastNoise/Metadata.cpp
    TARGET_DIRECTORY FastNoise PROPERTIES COMPILE_OPTIONS -fno-sanitize=alignment)

if(MAYHEM_KAT)
    add_executable(kat kat/kat.cpp)
    target_include_directories(kat PRIVATE ${FastNoise2_SOURCE_DIR}/src)
    target_link_libraries(kat PRIVATE FastNoise)
    return()
endif()

separate_arguments(fuzz_link_flags UNIX_COMMAND "${MAYHEM_FUZZ_LINK_FLAGS}")

add_library(standalone_main OBJECT ${MAYHEM_STANDALONE_MAIN})
set_source_files_properties(${MAYHEM_STANDALONE_MAIN} PROPERTIES LANGUAGE C)
add_library(lsan_off OBJECT ${MAYHEM_LSAN_OFF})

foreach(t fuzz_encoded_tree fuzz_node_tree_binary fuzz_nodedata fuzz_generate)
    add_executable(${t} harnesses/${t}.cpp $<TARGET_OBJECTS:lsan_off>)
    target_include_directories(${t} PRIVATE ${FastNoise2_SOURCE_DIR}/src harnesses)
    target_link_libraries(${t} PRIVATE FastNoise ${fuzz_link_flags})

    add_executable(${t}-standalone harnesses/${t}.cpp $<TARGET_OBJECTS:lsan_off> $<TARGET_OBJECTS:standalone_main>)
    target_include_directories(${t}-standalone PRIVATE ${FastNoise2_SOURCE_DIR}/src harnesses)
    target_link_libraries(${t}-standalone PRIVATE FastNoise)
endforeach()
