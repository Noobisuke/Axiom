cmake_minimum_required(VERSION 3.25)
project(Axiom LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- Compiler detection & flags ----------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Common warning flags
  set(COMMON_WARNING_FLAGS
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion
    -Wformat=2 -Wmisleading-indentation
    -Wduplicated-cond -Wduplicated-branches
    -Wlogical-op -Wuseless-cast
  )

  # Release: max optimization, target Zen 3 (znver3)
  # Use -ffat-lto-objects to avoid MinGW LTO linking issues with static libs
  set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffat-lto-objects -march=znver3 -mtune=znver3 -DNDEBUG"
  )

  # Debug: sanitizers, no optimization, full debug info
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer"
  )

  # Link flags for sanitizers in Debug
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG
    "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined"
  )

  # PEXT/BMI2 intrinsics need -mbmi2 (enabled by -march=znver3)
  # For portability, we also define a compile-time switch
  add_compile_options(${COMMON_WARNING_FLAGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # MSVC flags (unlikely here, but included for completeness)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /arch:AVX2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /fsanitize=address /RTC1")
  add_compile_options(/W4 /permissive- /std:c++20)
endif()

# ---------- Output directories ----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------- Engine library ----------
add_library(chess_engine STATIC
  src/bitboard.cpp
  src/position.cpp
  src/move.cpp
  src/movegen.cpp
  src/search.cpp
  src/eval.cpp
  src/tt.cpp
  src/uci.cpp
)

target_include_directories(chess_engine PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_definitions(chess_engine PRIVATE
  ENGINE_BUILD
)

# ---------- Main executable ----------
add_executable(axiom src/main.cpp)
target_link_libraries(axiom PRIVATE chess_engine)

# ---------- Perft test executable ----------
add_executable(perft_test tests/perft_test.cpp)
target_link_libraries(perft_test PRIVATE chess_engine)

# ---------- FEN / Position test executable ----------
add_executable(fen_test tests/fen_test.cpp)
target_link_libraries(fen_test PRIVATE chess_engine)

# ---------- Eval test executable ----------
add_executable(eval_test tests/eval_test.cpp)
target_link_libraries(eval_test PRIVATE chess_engine)

# ---------- Search test executable ----------
add_executable(search_test tests/search_test.cpp)
target_link_libraries(search_test PRIVATE chess_engine)

# ---------- CTest registration ----------
enable_testing()
add_test(NAME fen   COMMAND fen_test)
add_test(NAME eval  COMMAND eval_test)
add_test(NAME search COMMAND search_test)
add_test(NAME perft COMMAND perft_test 4)

# ---------- Bench executable ----------
add_executable(bench bench/bench.cpp)
target_link_libraries(bench PRIVATE chess_engine)

# ---------- Install (optional) ----------
install(TARGETS axiom RUNTIME DESTINATION bin)
install(TARGETS chess_engine ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.hpp")