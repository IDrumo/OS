#include "../include/background_launcher.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #define SLEEP_COMMAND "timeout"
    #define SLEEP_ARG "3"
#else
    #define SLEEP_COMMAND "sleep"
    #define SLEEP_ARG "3"
#endif

void testSimpleLaunch() {
    std::cout << "\n=== Тест 1: Простой запуск в фоне ===" << std::endl;
    
    std::string command = SLEEP_COMMAND " " SLEEP_ARG;
    
    std::cout << "Запускаем: " << command << std::endl;
    std::cout << "Программа запустится в фоне, тест продолжит работу сразу." << std::endl;
    
    int result = BackgroundLauncher::launch(command, false);
    
    if (result == 0) {
        std::cout << "Программа успешно запущена в фоне." << std::endl;
    } else {
        std::cout << "Ошибка запуска программы." << std::endl;
    }
}

void testLaunchAndWait() {
    std::cout << "\n=== Тест 2: Запуск с ожиданием ===" << std::endl;
    
    std::string command = SLEEP_COMMAND " " SLEEP_ARG;
    
    std::cout << "Запускаем: " << command << std::endl;
    std::cout << "Ожидаем завершения..." << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    int result = BackgroundLauncher::launch(command, true);
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Программа завершилась с кодом: " << result << std::endl;
    std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;
}

void testAdvancedControl() {
    std::cout << "\n=== Тест 3: Расширенное управление процессом ===" << std::endl;
    
    std::string command = SLEEP_COMMAND " 5";
    
    std::cout << "Запускаем: " << command << std::endl;
    
    BackgroundLauncher::ProcessId pid;
    BackgroundLauncher::ProcessHandle handle = BackgroundLauncher::launchWithControl(command, pid);
    
    if (
        #ifdef _WIN32
            handle == NULL || handle == INVALID_HANDLE_VALUE
        #else
            handle <= 0
        #endif
    ) {
        std::cout << "Ошибка запуска процесса." << std::endl;
        return;
    }
    
    std::cout << "PID процесса: " << pid << std::endl;
    
    // Проверяем статус сразу после запуска
    int exit_code;
    bool completed = BackgroundLauncher::isCompleted(handle, pid, exit_code);
    
    if (completed) {
        std::cout << "Процесс уже завершился. Код: " << exit_code << std::endl;
    } else {
        std::cout << "Процесс выполняется..." << std::endl;
    }
    
    // Ждем 2 секунды и проверяем снова
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    completed = BackgroundLauncher::isCompleted(handle, pid, exit_code);
    
    if (completed) {
        std::cout << "Процесс завершился через 2 секунды. Код: " << exit_code << std::endl;
    } else {
        std::cout << "Процесс все еще выполняется через 2 секунды." << std::endl;
    }
    
    // Ждем завершения с таймаутом
    std::cout << "Ожидаем завершения (таймаут 1 секунда)..." << std::endl;
    int result = BackgroundLauncher::waitForCompletion(handle, pid, 1000);
    
    if (result == -2) {
        std::cout << "Таймаут! Процесс все еще выполняется." << std::endl;
        
        // Принудительно завершаем процесс
        std::cout << "Принудительно завершаем процесс..." << std::endl;
        BackgroundLauncher::terminateProcess(handle, pid, true);
        
        // Ждем завершения
        result = BackgroundLauncher::waitForCompletion(handle, pid, 2000);
        std::cout << "Процесс завершен с кодом: " << result << std::endl;
    } else if (result >= 0) {
        std::cout << "Процесс завершился с кодом: " << result << std::endl;
    } else {
        std::cout << "Ошибка ожидания завершения." << std::endl;
    }
    
    // Закрываем дескриптор
    BackgroundLauncher::closeHandle(handle);
    std::cout << "Дескриптор процесса закрыт." << std::endl;
}

void testExternalProgram() {
    std::cout << "\n=== Тест 4: Запуск внешней программы ===" << std::endl;
    
    #ifdef _WIN32
        std::string command = "cmd /c echo Hello from Windows && dir /b";
    #else
        std::string command = "echo 'Hello from Linux' && ls -la | head -5";
    #endif
    
    std::cout << "Запускаем: " << command << std::endl;
    std::cout << "Ожидаем завершения..." << std::endl;
    
    int result = BackgroundLauncher::launch(command, true);
    
    std::cout << "Программа завершилась с кодом: " << result << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Тестирование библиотеки BackgroundLauncher ===" << std::endl;
    std::cout << "Платформа: " << 
        #ifdef _WIN32
            "Windows"
        #else
            "UNIX/POSIX"
        #endif
        << std::endl;
    
    try {
        testSimpleLaunch();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        testLaunchAndWait();
        
        testAdvancedControl();
        
        testExternalProgram();
        
        std::cout << "\n=== Все тесты завершены ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}