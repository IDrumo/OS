cmake_minimum_required(VERSION 3.10)
project(ProcessManager)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
set(SOURCES
        src/main.cpp
        src/platform.cpp
)

# Исполняемый файл
add_executable(app ${SOURCES})

# Подключаем заголовки
target_include_directories(app PRIVATE include)

# Настройки для MinGW
if(MINGW)
    target_compile_definitions(app PRIVATE
            _GLIBCXX_USE_C99_STDIO=1
            _WIN32_WINNT=0x0600)
endif()

# Настройки платформы
if(WIN32)
    target_link_libraries(app PRIVATE kernel32)
else()
    target_link_libraries(app PRIVATE pthread rt)
endif()