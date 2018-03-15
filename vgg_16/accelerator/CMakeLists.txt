cmake_minimum_required(VERSION 3.1)

project(accelerator)
include(../config.cmake)

file(GLOB SRC_FILES "*.cpp" "*.hpp")
add_library(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME})

add_subdirectory(test) #directory for test executable
