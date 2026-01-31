cmake_minimum_required(VERSION 3.10)
project(WeatherStation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

find_package(Qt5Charts QUIET)
if(NOT Qt5Charts_FOUND)
    message(WARNING "Qt5Charts не найден. Установите: sudo apt install libqt5charts5-dev")
endif()

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

add_executable(weather_gui
        gui/main_gui.cpp
        gui/gui.cpp
)

target_include_directories(weather_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

if(Qt5Charts_FOUND)
    target_link_libraries(weather_gui
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
            Qt5::Charts
    )
else()
    target_link_libraries(weather_gui
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
    )
    message(WARNING "GUI собран без поддержки Charts")
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(weather_server dl pthread)
    target_link_libraries(sensor_simulator dl pthread)
    target_link_libraries(weather_gui dl pthread)
elseif(WIN32)
    target_link_libraries(weather_server ws2_32)
    target_link_libraries(weather_gui ws2_32)
endif()