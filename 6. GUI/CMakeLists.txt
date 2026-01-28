cmake_minimum_required(VERSION 3.16)
project(TemperatureMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Настройки для Qt
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)
    set(QT_VERSION 5)
else()
    set(QT_VERSION 6)
endif()

find_package(QWT REQUIRED)

add_executable(temp_logger src/main.cpp third_party/sqlite/sqlite3.c)

# Симулятор (как было)
add_executable(simulator src/simulator.cpp)

# Новое GUI приложение
add_executable(temp_gui
        src/gui_main.cpp
        src/temp_monitor_gui.cpp
        src/temp_monitor_gui.h
)

# Подключаем зависимости для GUI приложения
if(QT_VERSION EQUAL 6)
    target_link_libraries(temp_gui
            Qt6::Core
            Qt6::Widgets
            Qt6::Network
            ${QWT_LIBRARIES}
    )
else()
    target_link_libraries(temp_gui
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
            ${QWT_LIBRARIES}
    )
endif()

# Включаем директории
target_include_directories(temp_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${QWT_INCLUDE_DIR}
)

# Для Windows
if(WIN32)
    target_link_libraries(temp_logger ws2_32)
    target_compile_definitions(temp_logger PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)

    # Для GUI приложения на Windows
    target_compile_definitions(temp_gui PRIVATE QT_NO_KEYWORDS)

    # Копируем DLL файлы Qt и Qwt
    add_custom_command(TARGET temp_gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${QWT_LIBRARIES}>
            $<TARGET_FILE_DIR:temp_gui>/
            COMMENT "Copying QWT DLL to build directory"
    )
endif()

# Копируем index.html для сервера
add_custom_command(TARGET temp_logger POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
        COMMENT "Copying index.html to build directory"
)