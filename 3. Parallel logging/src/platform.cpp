#include "../include/platform.h"
#include <cstring>
#include <cerrno>

// ==================== РЕАЛИЗАЦИЯ ДЛЯ ВСЕХ ПЛАТФОРМ ====================

// ---------- Разделяемая память ----------
SharedMemoryHandle create_shared_memory(const char* name, size_t size) {
#ifdef _WIN32
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        (DWORD)size,
        name
    );

    if (hMapFile == NULL) {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
        return NULL;
    }

    return hMapFile;
#else
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
        return nullptr;
    }

    if (ftruncate(fd, size) == -1) {
        std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
        close(fd);
        return nullptr;
    }

    // Обертка для POSIX
    struct PosixSharedMemory* shm = new PosixSharedMemory;
    shm->fd = fd;
    shm->addr = nullptr;
    return shm;
#endif
}

void* map_shared_memory(SharedMemoryHandle handle) {
#ifdef _WIN32
    return MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
    PosixSharedMemory* shm = static_cast<PosixSharedMemory*>(handle);
    struct stat st;
    fstat(shm->fd, &st);
    shm->addr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);

    if (shm->addr == MAP_FAILED) {
        return nullptr;
    }

    return shm->addr;
#endif
}

void unmap_shared_memory(SharedMemoryHandle handle, void* addr) {
#ifdef _WIN32
    UnmapViewOfFile(addr);
#else
    if (handle && addr) {
        PosixSharedMemory* shm = static_cast<PosixSharedMemory*>(handle);
        struct stat st;
        fstat(shm->fd, &st);
        munmap(addr, st.st_size);
    }
#endif
}

void close_shared_memory(SharedMemoryHandle handle) {
#ifdef _WIN32
    CloseHandle(handle);
#else
    if (handle) {
        PosixSharedMemory* shm = static_cast<PosixSharedMemory*>(handle);
        if (shm->fd != -1) close(shm->fd);
        delete shm;
    }
#endif
}

// ---------- Семафоры ----------
SemaphoreHandle create_semaphore(const char* name, int initial) {
#ifdef _WIN32
    return CreateSemaphore(NULL, initial, 1, name);
#else
    sem_t* sem = sem_open(name, O_CREAT, 0666, initial);
    if (sem == SEM_FAILED) {
        std::cerr << "sem_open failed: " << strerror(errno) << std::endl;
        return nullptr;
    }

    PosixSemaphore* psem = new PosixSemaphore;
    psem->sem = sem;
    return psem;
#endif
}

void wait_semaphore(SemaphoreHandle sem) {
#ifdef _WIN32
    WaitForSingleObject(sem, INFINITE);
#else
    if (sem) sem_wait(static_cast<PosixSemaphore*>(sem)->sem);
#endif
}

void post_semaphore(SemaphoreHandle sem) {
#ifdef _WIN32
    ReleaseSemaphore(sem, 1, NULL);
#else
    if (sem) sem_post(static_cast<PosixSemaphore*>(sem)->sem);
#endif
}

void close_semaphore(SemaphoreHandle sem) {
#ifdef _WIN32
    CloseHandle(sem);
#else
    if (sem) {
        PosixSemaphore* psem = static_cast<PosixSemaphore*>(sem);
        sem_close(psem->sem);
        delete psem;
    }
#endif
}

// ---------- Мьютексы ----------
MutexHandle create_mutex(const char* name) {
#ifdef _WIN32
    return CreateMutex(NULL, FALSE, name);
#else
    PosixMutex* mutex = new PosixMutex;
    pthread_mutexattr_init(&mutex->attr);
    pthread_mutexattr_setpshared(&mutex->attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex->mutex, &mutex->attr);
    return mutex;
#endif
}

void lock_mutex(MutexHandle mutex) {
#ifdef _WIN32
    WaitForSingleObject(mutex, INFINITE);
#else
    if (mutex) pthread_mutex_lock(&static_cast<PosixMutex*>(mutex)->mutex);
#endif
}

void unlock_mutex(MutexHandle mutex) {
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    if (mutex) pthread_mutex_unlock(&static_cast<PosixMutex*>(mutex)->mutex);
#endif
}

void close_mutex(MutexHandle mutex) {
#ifdef _WIN32
    CloseHandle(mutex);
#else
    if (mutex) {
        PosixMutex* pmutex = static_cast<PosixMutex*>(mutex);
        pthread_mutex_destroy(&pmutex->mutex);
        pthread_mutexattr_destroy(&pmutex->attr);
        delete pmutex;
    }
#endif
}

// ---------- Барьеры ----------
BarrierHandle create_barrier(int count) {
#ifdef _WIN32
    #if _WIN32_WINNT >= 0x0602
        SYNCHRONIZATION_BARRIER* barrier = new SYNCHRONIZATION_BARRIER;
        if (InitializeSynchronizationBarrier(barrier, count, -1)) {
            return barrier;
        }
    #endif
    return nullptr;
#else
    PosixBarrier* barrier = new PosixBarrier;
    pthread_barrier_init(&barrier->barrier, NULL, count);
    return barrier;
#endif
}

void wait_barrier(BarrierHandle barrier) {
#ifdef _WIN32
    #if _WIN32_WINNT >= 0x0602
        EnterSynchronizationBarrier(barrier, 0);
    #endif
#else
    if (barrier) pthread_barrier_wait(&static_cast<PosixBarrier*>(barrier)->barrier);
#endif
}

void close_barrier(BarrierHandle barrier) {
#ifdef _WIN32
    #if _WIN32_WINNT >= 0x0602
        DeleteSynchronizationBarrier(barrier);
        delete barrier;
    #endif
#else
    if (barrier) {
        PosixBarrier* pbarrier = static_cast<PosixBarrier*>(barrier);
        pthread_barrier_destroy(&pbarrier->barrier);
        delete pbarrier;
    }
#endif
}

// ---------- Потоки ----------
struct ThreadData {
    void* (*func)(void*);
    void* arg;
};

#ifdef _WIN32
DWORD WINAPI thread_wrapper_win(LPVOID param) {
    ThreadData* data = static_cast<ThreadData*>(param);
    void* result = data->func(data->arg);
    delete data;
    // Исправленное приведение типов для 64-битного MinGW
    return static_cast<DWORD>(reinterpret_cast<uintptr_t>(result));
}
#else
void* thread_wrapper_posix(void* param) {
    ThreadData* data = static_cast<ThreadData*>(param);
    void* result = data->func(data->arg);
    delete data;
    return result;
}
#endif

ThreadHandle create_thread(void* (*func)(void*), void* arg) {
    ThreadData* data = new ThreadData{func, arg};

#ifdef _WIN32
    DWORD threadId;
    HANDLE thread = CreateThread(NULL, 0, thread_wrapper_win, data, 0, &threadId);
    if (thread == NULL) {
        delete data;
        return NULL;
    }
    return thread;
#else
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_wrapper_posix, data) != 0) {
        delete data;
        return 0;
    }
    return thread;
#endif
}

void join_thread(ThreadHandle thread) {
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

// ---------- Утилиты ----------
ProcessID get_current_pid() {
    return GET_PID();  // Используем макрос из platform.h
}

void sleep_ms(int milliseconds) {
    SLEEP_MS(milliseconds);  // Используем макрос из platform.h
}

// ---------- Запуск дочернего процесса ----------
bool spawn_child_process(const char* program, char* const argv[]) {
#ifdef _WIN32
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // Создаем командную строку
    std::string cmd = program;
    for (int i = 1; argv[i] != nullptr; i++) {
        cmd += " ";
        cmd += argv[i];
    }

    if (CreateProcess(NULL, const_cast<LPSTR>(cmd.c_str()),
                     NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
#else
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        execvp(program, argv);
        exit(1);  // Если execvp не удался
    } else if (pid > 0) {
        return true;
    }
    return false;
#endif
}

// ---------- POSIX структуры (только для POSIX) ----------
#ifndef _WIN32
struct PosixMutex {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
};

struct PosixSemaphore {
    sem_t* sem;
};

struct PosixSharedMemory {
    int fd;
    void* addr;
};

struct PosixBarrier {
    pthread_barrier_t barrier;
};
#endif