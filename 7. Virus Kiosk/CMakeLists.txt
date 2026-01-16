cmake_minimum_required(VERSION 3.10)
project(app VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(${PROJECT_NAME} test/test_launcher.cpp)
target_link_libraries(${PROJECT_NAME} background_launcher)
