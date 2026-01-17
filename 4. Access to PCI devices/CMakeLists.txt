cmake_minimum_required(VERSION 3.10)
project(TemperatureMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TEMP_MONITOR_SOURCES
        src/main.cpp
        src/config.cpp
        src/data_processor.cpp
        src/rotating_log.cpp
        src/serial.cpp
        src/temperature_measurement.cpp
        src/temperature_validator.cpp
)

set(SIMULATOR_SOURCES
        src/simulator.cpp
        src/serial.cpp
)

add_executable(temperature_monitor ${TEMP_MONITOR_SOURCES})
target_include_directories(temperature_monitor PRIVATE include)

add_executable(simulator ${SIMULATOR_SOURCES})
target_include_directories(simulator PRIVATE include)

add_executable(test_monitor
        src/test_monitor.cpp
        src/config.cpp
        src/data_processor.cpp
        src/rotating_log.cpp
        src/temperature_measurement.cpp
        src/temperature_validator.cpp
)
target_include_directories(test_monitor PRIVATE include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(temperature_monitor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()