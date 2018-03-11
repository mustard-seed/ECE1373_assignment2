set(CONV_LAYER_TEST ${PROJECT_NAME}_test)
add_executable(${CONV_LAYER_TEST} ./convLayer_test.cpp)
target_link_libraries(${CONV_LAYER_TEST} ${PROJECT_NAME})
