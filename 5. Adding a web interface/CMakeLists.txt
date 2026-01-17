cmake_minimum_required(VERSION 3.10)
project(TemperatureLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Указываем компилятору искать заголовки в include директории
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite
)

# Основная программа
add_executable(temp_logger
        src/main.cpp
        src/database.cpp
        src/webserver.cpp
        src/serial_port.cpp
        third_party/sqlite/sqlite3.c
)

# Симулятор устройства
add_executable(simulator src/simulator.cpp)

# Для Windows добавляем библиотеки
if(WIN32)
    target_link_libraries(temp_logger ws2_32)
    target_compile_definitions(temp_logger PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
endif()

# Автоматически копируем web директорию в директорию сборки
# Создаем кастомную команду, которая выполняется после сборки
add_custom_command(TARGET temp_logger POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/web
        ${CMAKE_CURRENT_BINARY_DIR}/web
        COMMENT "Копирование веб-интерфейса в директорию сборки"
)

# Для симулятора тоже можно добавить, если нужно
add_custom_command(TARGET simulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Симулятор собран"
)

# Настраиваем установку (опционально, для make install)
install(TARGETS temp_logger simulator
        RUNTIME DESTINATION bin
)

install(DIRECTORY web/
        DESTINATION share/temp_logger/web
)