set(SIMPLE_TEST ${PROJECT_NAME}_simple_test)
add_executable(${SIMPLE_TEST} ./simple_test.cpp)
target_link_libraries(${SIMPLE_TEST} ${PROJECT_NAME})
