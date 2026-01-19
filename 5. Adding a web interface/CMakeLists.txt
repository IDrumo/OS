cmake_minimum_required(VERSION 3.10)
project(TemperatureLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Указываем компилятору искать заголовки
include_directories(
        third_party/sqlite
)

# Основная программа
add_executable(temp_logger src/main.cpp third_party/sqlite/sqlite3.c)

# Симулятор
add_executable(simulator src/simulator.cpp)

# Для Windows
if(WIN32)
    target_link_libraries(temp_logger ws2_32)
    target_compile_definitions(temp_logger PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()

# Копируем index.html в директорию сборки
add_custom_command(TARGET temp_logger POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
        COMMENT "Copying index.html to build directory"
)