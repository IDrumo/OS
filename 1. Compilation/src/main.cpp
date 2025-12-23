#include <iostream>

#ifdef _WIN32
    #include <windows.h>
auto OS_NAME = "Windows";
#elif __unix__
const char* OS_NAME = "Unix/Linux";
#elif __APPLE__
const char* OS_NAME = "macOS";
#else
const char* OS_NAME = "Unknown";
#endif

int main() {
    std::cout << "Hello, World!\n";
    std::cout << "Hello, World!\n";
    std::cout << "Определение операционной системы:\n";

    std::cout << "Вы работаете в системе: " << OS_NAME << "\n\n";

#ifdef _WIN32
    std::cout << "Дополнительная информация для Windows:\n";
    std::cout << "Версия Windows: ";

    DWORD dwVersion = GetVersion();
    DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    std::cout << dwMajorVersion << "." << dwMinorVersion << "\n";
#endif
    return 0;
}