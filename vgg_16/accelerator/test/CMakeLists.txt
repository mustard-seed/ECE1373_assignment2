set(ACCEL_TEST ${PROJECT_NAME}_test)
add_executable(${ACCEL_TEST} ./accelerator_test.cpp)
target_link_libraries(${ACCEL_TEST} ${PROJECT_NAME} shared)
