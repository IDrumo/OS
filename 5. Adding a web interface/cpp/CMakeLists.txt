cmake_minimum_required(VERSION 3.10)
project(TemperatureMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск PostgreSQL (libpq)
find_package(PostgreSQL REQUIRED)

# Поиск libpqxx
find_path(PQXX_INCLUDE_DIR pqxx/pqxx)
find_library(PQXX_LIBRARY pqxx)

if(NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIBRARY)
    message(FATAL_ERROR "libpqxx not found. Install it with: sudo apt-get install libpqxx-dev")
endif()

# httplib (header-only)
set(HTTPLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# nlohmann/json (header-only)
set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Основные исходные файлы
set(MAIN_SOURCES
        src/main.cpp
        src/config.cpp
        src/data_processor.cpp
        src/temperature_measurement.cpp
        src/temperature_validator.cpp
        src/serial.cpp
        src/database.cpp
        src/http_server.cpp
)

# Добавляем исполняемый файл
add_executable(temperature_monitor ${MAIN_SOURCES})

target_include_directories(temperature_monitor
        PRIVATE
        include
        ${HTTPLIB_INCLUDE_DIR}
        ${JSON_INCLUDE_DIR}
        ${PQXX_INCLUDE_DIR}
        ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(temperature_monitor
        PRIVATE
        ${PQXX_LIBRARY}
        ${PostgreSQL_LIBRARIES}
        pthread
)

# Симулятор
add_executable(simulator src/simulator.cpp src/serial.cpp)
target_include_directories(simulator PRIVATE include)

# Настройки компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(temperature_monitor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Для Windows
if(WIN32)
    # Дополнительные библиотеки для Windows
    target_link_libraries(temperature_monitor PRIVATE ws2_32)
endif()