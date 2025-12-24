#ifndef BACKGROUND_LAUNCHER_H
#define BACKGROUND_LAUNCHER_H

#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

class BackgroundLauncher {
public:
    #ifdef _WIN32
        typedef HANDLE ProcessHandle;
        typedef DWORD ProcessId;
    #else
        typedef pid_t ProcessHandle;
        typedef pid_t ProcessId;
    #endif

    /**
     * @brief Запускает программу в фоновом режиме
     * @param command Команда для выполнения (с аргументами)
     * @param wait_for_completion Ждать ли завершения программы
     * @return Код возврата программы (если wait_for_completion = true)
     *         или 0 если программа запущена в фоне (если wait_for_completion = false)
     */
    static int launch(const std::string& command, bool wait_for_completion = false);

    /**
     * @brief Запускает программу в фоновом режиме с возможностью управления процессом
     * @param command Команда для выполнения
     * @param process_id[out] Идентификатор запущенного процесса
     * @return Дескриптор процесса (HANDLE в Windows, pid_t в POSIX)
     */
    static ProcessHandle launchWithControl(const std::string& command, ProcessId& process_id);

    /**
     * @brief Ожидает завершения процесса и получает код возврата
     * @param process Дескриптор процесса, полученный из launchWithControl
     * @param process_id Идентификатор процесса
     * @param timeout_ms Таймаут ожидания в миллисекундах (0 - бесконечно)
     * @return Код возврата процесса или -1 в случае ошибки
     */
    static int waitForCompletion(ProcessHandle process, ProcessId process_id, int timeout_ms = 0);

    /**
     * @brief Проверяет, завершился ли процесс
     * @param process Дескриптор процесса
     * @param process_id Идентификатор процесса
     * @param exit_code[out] Код возврата (если процесс завершился)
     * @return true если процесс завершился, false если еще работает
     */
    static bool isCompleted(ProcessHandle process, ProcessId process_id, int& exit_code);

    /**
     * @brief Завершает процесс
     * @param process Дескриптор процесса
     * @param process_id Идентификатор процесса
     * @param force Принудительное завершение (SIGKILL/terminate)
     */
    static void terminateProcess(ProcessHandle process, ProcessId process_id, bool force = false);

    /**
     * @brief Закрывает дескриптор процесса
     * @param process Дескриптор процесса
     */
    static void closeHandle(ProcessHandle process);

private:
    #ifdef _WIN32
    static std::wstring stringToWstring(const std::string& str);
    static std::string getLastErrorString();
    #endif
};

#endif // BACKGROUND_LAUNCHER_H