cmake_minimum_required(VERSION 3.10)
project(WeatherStation)

set(CMAKE_CXX_STANDARD 17)

add_executable(weather_server
        core/main_core.cpp
        core/core.cpp
        database/database.cpp
        database/sqlite3.c
        web/web.cpp
)

target_include_directories(weather_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/database
        ${CMAKE_CURRENT_SOURCE_DIR}/web
)

configure_file(web/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html COPYONLY)

add_executable(sensor_simulator
        core/main_sim.cpp
        core/core.cpp
)

target_include_directories(sensor_simulator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

if(WIN32)
    target_link_libraries(weather_server ws2_32)
else()
    target_link_libraries(weather_server pthread)
    target_link_libraries(sensor_simulator pthread)
endif()