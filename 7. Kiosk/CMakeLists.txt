cmake_minimum_required(VERSION 3.10)
project(WeatherStation)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 COMPONENTS Core Widgets Network Charts REQUIRED)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets Network Charts REQUIRED)
    set(QT_LIBRARIES Qt5::Widgets Qt5::Network Qt5::Charts)
else()
    set(QT_LIBRARIES Qt6::Widgets Qt6::Network Qt6::Charts)
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
        gui/gui.h
)

set_target_properties(weather_gui PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
)

target_include_directories(weather_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

#target_link_libraries(weather_gui ${QT_LIBRARIES})
target_link_libraries(weather_gui
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

if(WIN32)
    target_link_libraries(weather_server ws2_32)
    target_link_libraries(weather_gui -lws2_32)
else()
    target_link_libraries(weather_server pthread)
    target_link_libraries(sensor_simulator pthread)
    target_link_libraries(weather_gui pthread)
endif()