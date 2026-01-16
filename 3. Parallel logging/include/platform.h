#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string>
#include <iostream>

// ==================== ОПРЕДЕЛЕНИЕ ПЛАТФОРМЫ ====================
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_POSIX 0

    #define _WIN32_WINNT 0x0600
    #include <windows.h>
    #include <synchapi.h>

    // Windows типы
    typedef DWORD ProcessID;
    typedef HANDLE ThreadHandle;
    typedef HANDLE MutexHandle;
    typedef HANDLE SemaphoreHandle;
    typedef HANDLE SharedMemoryHandle;
    typedef HANDLE BarrierHandle;

    // Windows функции
    #define GET_PID() GetCurrentProcessId()
    #define SLEEP_MS(ms) Sleep(ms)
    #define CREATE_PROCESS(cmd) CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)

    // Имена объектов
    #define SHM_NAME "Local\\MyAppSharedMemory"
    #define SEM_NAME "Global\\MyAppSemaphore"
    #define MUTEX_NAME "Global\\MyAppMutex"
    #define BARRIER_NAME "Global\\MyAppBarrier"

#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_POSIX 1

    #include <pthread.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>

    // POSIX типы
    typedef pid_t ProcessID;
    typedef pthread_t ThreadHandle;

    // POSIX структуры (скрыты в .cpp)
    struct PosixMutex;
    struct PosixSemaphore;
    struct PosixSharedMemory;
    struct PosixBarrier;

    typedef PosixMutex* MutexHandle;
    typedef PosixSemaphore* SemaphoreHandle;
    typedef PosixSharedMemory* SharedMemoryHandle;
    typedef PosixBarrier* BarrierHandle;

    // POSIX функции
    #define GET_PID() getpid()
    #define SLEEP_MS(ms) usleep((ms) * 1000)
    #define CREATE_PROCESS(cmd) (fork() == 0 ? execvp(cmd[0], cmd) : -1)

    // Имена объектов
    #define SHM_NAME "/myapp_shm"
    #define SEM_NAME "/myapp_sem"
    #define MUTEX_NAME "/myapp_mutex"
    #define BARRIER_NAME NULL
#endif

// ==================== УНИВЕРСАЛЬНЫЕ МАКРОСЫ ====================
// Эти макросы работают одинаково на всех платформах
#define LOCK(mutex) lock_mutex(mutex)
#define UNLOCK(mutex) unlock_mutex(mutex)
#define WAIT(sem) wait_semaphore(sem)
#define POST(sem) post_semaphore(sem)

// ==================== ОБЩИЕ ОБЪЯВЛЕНИЯ ФУНКЦИЙ ====================
// Все функции реализованы в platform.cpp с #ifdef внутри

// Разделяемая память
SharedMemoryHandle create_shared_memory(const char* name, size_t size);
void* map_shared_memory(SharedMemoryHandle handle);
void unmap_shared_memory(SharedMemoryHandle handle, void* addr);
void close_shared_memory(SharedMemoryHandle handle);

// Семафоры
SemaphoreHandle create_semaphore(const char* name, int initial);
void wait_semaphore(SemaphoreHandle sem);
void post_semaphore(SemaphoreHandle sem);
void close_semaphore(SemaphoreHandle sem);

// Мьютексы
MutexHandle create_mutex(const char* name);
void lock_mutex(MutexHandle mutex);
void unlock_mutex(MutexHandle mutex);
void close_mutex(MutexHandle mutex);

// Барьеры
BarrierHandle create_barrier(int count);
void wait_barrier(BarrierHandle barrier);
void close_barrier(BarrierHandle barrier);

// Потоки
ThreadHandle create_thread(void* (*func)(void*), void* arg);
void join_thread(ThreadHandle thread);

// Прочие утилиты
ProcessID get_current_pid();
void sleep_ms(int milliseconds);

// Запуск дочернего процесса (универсальный интерфейс)
bool spawn_child_process(const char* program, char* const argv[]);

#endif // PLATFORM_H