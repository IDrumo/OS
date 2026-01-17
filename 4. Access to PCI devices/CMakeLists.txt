cmake_minimum_required(VERSION 3.10)
project(TemperatureLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Основная программа
add_executable(temp_logger src/main.cpp)

# Симулятор устройства
add_executable(device_simulator src/device_simulator.cpp)

# библиотеки для работы с портами
target_link_libraries(temp_logger)