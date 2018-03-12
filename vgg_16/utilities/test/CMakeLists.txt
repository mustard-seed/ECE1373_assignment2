set(UTILITIES_TEST ${PROJECT_NAME}_test)
add_executable(${UTILITIES_TEST} ./utilities_test.cpp)
target_link_libraries(${UTILITIES_TEST} ${PROJECT_NAME})
