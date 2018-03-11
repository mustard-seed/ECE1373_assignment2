# Additional target to perform clang-format/clang-tidy run
# Requires clang-format and clang-tidy

# Get all project files
file(GLOB_RECURSE ALL_SOURCE_FILES *.c *.cpp)
file(GLOB_RECURSE REMOVE_SOURCES "${CMAKE_BINARY_DIR}/*" "build/*")
list(REMOVE_ITEM ALL_SOURCE_FILES ${REMOVE_SOURCES})
file(GLOB_RECURSE ALL_HEADER_FILES *.hpp *.h)

add_custom_target(
        clang-format
        COMMAND /usr/bin/clang-format-4.0
        -style=file
        -fallback-style=WebKit
        -i
        -sort-includes
        ${ALL_SOURCE_FILES}
)

set(CLANG_TIDY_CHECKS "cppcoreguidelines-*,clang-analyzer-*,google-*,llvm-*,modernize-*,performance-*,readability-*,bugprone-*,misc-*,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-cppcoreguidelines-pro-type-vararg,-llvm-namespace-comment,-google-readability-namespace-comments,-modernize-use-bool-literals,-llvm-include-order,-modernize-raw-string-literal,-google-build-using-namespace,-cppcoreguidelines-pro-bounds-constant-array-index")
add_custom_target(
        clang-tidy
        COMMAND run-clang-tidy-4.0.py -checks=${CLANG_TIDY_CHECKS} -header-filter="^include" -p ${CMAKE_BINARY_DIR} 1>&2
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set(CLANG_TIDY_FIXES "google-readability-braces-around-statements,readability-else-after-return,cppcoreguidelines-pro-type-member-init,google-readability-casting,readability-implicit-bool-cast,modernize-loop-convert,readability-redundant-string-init,readability-simplify-boolean-expr,readability-redundant-member-init,google-runtime-int")
add_custom_target(
        clang-tidy-fix
        COMMAND run-clang-tidy-4.0.py -checks=${CLANG_TIDY_FIXES} -header-filter="^include" -p ${CMAKE_BINARY_DIR} -fix 1>&2
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
