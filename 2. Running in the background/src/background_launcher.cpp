#include "../include/background_launcher.h"
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <cstring>

#ifdef _WIN32
    #include <tchar.h>
    #include <shellapi.h>
#else
    #include <signal.h>
    #include <errno.h>
#endif

int BackgroundLauncher::launch(const std::string& command, bool wait_for_completion) {
    if (wait_for_completion) {
        #ifdef _WIN32
            return system(command.c_str());
        #else
            return system(command.c_str());
        #endif
    } else {
        ProcessId pid;
        ProcessHandle handle = launchWithControl(command, pid);

        if (
            #ifdef _WIN32
                handle == NULL || handle == INVALID_HANDLE_VALUE
            #else
                handle <= 0
            #endif
        ) {
            return -1;
        }

        closeHandle(handle);
        return 0;
    }
}

BackgroundLauncher::ProcessHandle BackgroundLauncher::launchWithControl(const std::string& command, ProcessId& process_id) {
    #ifdef _WIN32
        std::wstring wcmd = stringToWstring(command);
        
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        wchar_t* cmdline = _wcsdup(wcmd.c_str());

        DWORD creationFlags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;

        if (!CreateProcessW(
            NULL,           // Имя приложения (используем командную строку)
            cmdline,        // Командная строка
            NULL,           // Атрибуты защиты процесса
            NULL,           // Атрибуты защиты потока
            FALSE,          // Наследование дескрипторов
            creationFlags,  // Флаги создания
            NULL,           // Окружение (используем родительское)
            NULL,           // Текущий каталог
            &si,            // STARTUPINFO
            &pi             // PROCESS_INFORMATION
        )) {
            free(cmdline);
            std::cerr << "CreateProcess failed: " << getLastErrorString() << std::endl;
            process_id = 0;
            return INVALID_HANDLE_VALUE;
        }
        
        free(cmdline);

        CloseHandle(pi.hThread);
        
        process_id = pi.dwProcessId;
        return pi.hProcess;
        
    #else
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << "fork failed: " << strerror(errno) << std::endl;
            process_id = -1;
            return -1;
        } else if (pid == 0) {
            signal(SIGINT, SIG_IGN);

            std::vector<char*> args;
            std::istringstream iss(command);
            std::string token;
            
            while (iss >> token) {
                char* arg = new char[token.size() + 1];
                strcpy(arg, token.c_str());
                args.push_back(arg);
            }
            args.push_back(nullptr);

            execvp(args[0], args.data());
            
            std::cerr << "execvp failed: " << strerror(errno) << std::endl;
            
            for (char* arg : args) {
                if (arg) delete[] arg;
            }
            
            _exit(EXIT_FAILURE);
        } else {
            process_id = pid;
            return pid;
        }
    #endif
}

int BackgroundLauncher::waitForCompletion(ProcessHandle process, ProcessId process_id, int timeout_ms) {
    #ifdef _WIN32
        if (process == NULL || process == INVALID_HANDLE_VALUE) {
            return -1;
        }
        
        DWORD waitResult;
        if (timeout_ms > 0) {
            waitResult = WaitForSingleObject(process, timeout_ms);
        } else {
            waitResult = WaitForSingleObject(process, INFINITE);
        }
        
        if (waitResult == WAIT_OBJECT_0) {
            DWORD exitCode;
            if (GetExitCodeProcess(process, &exitCode)) {
                return static_cast<int>(exitCode);
            }
        } else if (waitResult == WAIT_TIMEOUT) {
            return -2;
        }
        
        return -1;
        
    #else
        int status;
        pid_t result;
        
        if (timeout_ms > 0) {
            for (int i = 0; i < timeout_ms / 100; i++) {
                result = waitpid(process_id, &status, WNOHANG);
                if (result == process_id) {
                    break;
                } else if (result == 0) {
                    usleep(100000); // 100ms
                } else {
                    return -1;
                }
            }
            
            if (result == 0) {
                return -2;
            }
        } else {
            result = waitpid(process_id, &status, 0);
        }
        
        if (result == process_id) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
        }
        
        return -1;
    #endif
}

bool BackgroundLauncher::isCompleted(ProcessHandle process, ProcessId process_id, int& exit_code) {
    #ifdef _WIN32
        DWORD dwExitCode;
        if (GetExitCodeProcess(process, &dwExitCode)) {
            if (dwExitCode == STILL_ACTIVE) {
                return false;
            } else {
                exit_code = static_cast<int>(dwExitCode);
                return true;
            }
        }
        return false;
    #else
        int status;
        pid_t result = waitpid(process_id, &status, WNOHANG);
        
        if (result == 0) {
            return false;
        } else if (result == process_id) {
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                exit_code = 128 + WTERMSIG(status);
            } else {
                exit_code = -1;
            }
            return true;
        }
        
        return false;
    #endif
}

void BackgroundLauncher::terminateProcess(ProcessHandle process, ProcessId process_id, bool force) {
    #ifdef _WIN32
        if (force) {
            TerminateProcess(process, 1);
        } else {
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, process_id);
        }
    #else
        if (force) {
            kill(process_id, SIGKILL);
        } else {
            kill(process_id, SIGTERM);
        }
    #endif
}

void BackgroundLauncher::closeHandle(ProcessHandle process) {
    #ifdef _WIN32
        if (process && process != INVALID_HANDLE_VALUE) {
            CloseHandle(process);
        }
    #else
        (void)process;
    #endif
}

#ifdef _WIN32
std::wstring BackgroundLauncher::stringToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string BackgroundLauncher::getLastErrorString() {
    DWORD error = GetLastError();
    if (error == 0) return "";
    
    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buffer, 0, NULL);
    
    std::string message(buffer, size);
    LocalFree(buffer);
    
    return message;
}
#endif