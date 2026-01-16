#include "../include/platform.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <cstring>
#include <limits>
#include <atomic>

const char* LOG_FILE = "app.log";

// Структура для разделяемых данных
struct SharedData {
    volatile int32_t counter;
    volatile int32_t leader_pid;
    volatile int32_t copy1_pid;
    volatile int32_t copy2_pid;
    volatile bool copy1_running;
    volatile bool copy2_running;
    volatile int64_t last_copy1_start;
    volatile int64_t last_copy2_start;
};

class Application {
private:
    SharedData* shared_data;
    SharedMemoryHandle shm_handle;
    SemaphoreHandle semaphore;
    MutexHandle log_mutex;
    std::ofstream log_file;
    bool is_leader;
    bool running;

public:
    Application() : shared_data(nullptr), is_leader(false), running(true) {
        open_log_file();
        init_shared_memory();
        init_synchronization();
        determine_leader();
        log_start();
    }

    ~Application() {
        cleanup();
    }

    void run() {
        // Основные потоки
        ThreadHandle timer_thread = create_thread(timer_increment_wrapper, this);
        ThreadHandle input_thread = create_thread(input_listener_wrapper, this);

        if (is_leader) {
            ThreadHandle logger_thread = create_thread(logger_wrapper, this);
            ThreadHandle spawner_thread = create_thread(spawner_wrapper, this);

            // Ожидаем завершения потоков лидера
            join_thread(logger_thread);
            join_thread(spawner_thread);
        }

        // Ожидаем завершения общих потоков
        join_thread(timer_thread);
        join_thread(input_thread);
    }

    void stop() {
        running = false;
    }

    void do_copy_work(int copy_type) {
        WAIT(semaphore);
        if (copy_type == 1) {
            shared_data->counter += 10;
            std::cout << "Copy_1: Increased counter by 10. New value: "
                      << shared_data->counter << std::endl;
        } else if (copy_type == 2) {
            shared_data->counter *= 2;
            std::cout << "Copy_2: Multiplied counter by 2. New value: "
                      << shared_data->counter << std::endl;
        }
        POST(semaphore);

        if (copy_type == 2) {
            SLEEP_MS(2000);  // 2 секунды
            WAIT(semaphore);
            shared_data->counter /= 2;
            std::cout << "Copy_2: Divided counter by 2. New value: "
                      << shared_data->counter << std::endl;
            POST(semaphore);
        }
    }

    void mark_copy_finished(int copy_type) {
        WAIT(semaphore);
        if (copy_type == 1) {
            shared_data->copy1_running = false;
        } else {
            shared_data->copy2_running = false;
        }
        POST(semaphore);
    }

    static std::string get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        auto timer = std::chrono::system_clock::to_time_t(now);
        std::tm bt;

        #ifdef _WIN32
            localtime_s(&bt, &timer);
        #else
            localtime_r(&timer, &bt);
        #endif

        std::ostringstream oss;
        oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

private:
    static void* timer_increment_wrapper(void* arg) {
        Application* app = static_cast<Application*>(arg);
        app->timer_increment_thread();
        return nullptr;
    }

    static void* input_listener_wrapper(void* arg) {
        Application* app = static_cast<Application*>(arg);
        app->input_listener_thread();
        return nullptr;
    }

    static void* logger_wrapper(void* arg) {
        Application* app = static_cast<Application*>(arg);
        app->logger_thread();
        return nullptr;
    }

    static void* spawner_wrapper(void* arg) {
        Application* app = static_cast<Application*>(arg);
        app->spawner_thread();
        return nullptr;
    }

    void open_log_file() {
        log_file.open(LOG_FILE, std::ios_base::app);
        if (!log_file.is_open()) {
            std::cerr << "Cannot open log file!" << std::endl;
            exit(1);
        }
    }

    void init_shared_memory() {
        // Закрыть существующий handle, если есть
        #ifdef _WIN32
            HANDLE h = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
            if (h) {
                CloseHandle(h);
                SLEEP_MS(100);
            }
        #endif

        shm_handle = create_shared_memory(SHM_NAME, sizeof(SharedData));
        if (!shm_handle) {
            std::cerr << "Failed to create shared memory!" << std::endl;
            exit(1);
        }

        shared_data = static_cast<SharedData*>(map_shared_memory(shm_handle));
        if (!shared_data) {
            std::cerr << "Failed to map shared memory!" << std::endl;
            exit(1);
        }

        // Инициализация, если мы первые
        LOCK(log_mutex);
        if (shared_data->leader_pid == 0) {
            shared_data->counter = 0;
            shared_data->leader_pid = static_cast<int32_t>(GET_PID());
            shared_data->copy1_pid = 0;
            shared_data->copy2_pid = 0;
            shared_data->copy1_running = false;
            shared_data->copy2_running = false;
            shared_data->last_copy1_start = 0;
            shared_data->last_copy2_start = 0;

            std::cout << "Initialized shared memory. I am the leader." << std::endl;
        }
        UNLOCK(log_mutex);
    }

    void init_synchronization() {
        semaphore = create_semaphore(SEM_NAME, 1);
        log_mutex = create_mutex(MUTEX_NAME);
    }

    void determine_leader() {
        is_leader = (shared_data->leader_pid == static_cast<int32_t>(GET_PID()));
        std::cout << "I am " << (is_leader ? "LEADER" : "FOLLOWER")
                  << ". Leader PID: " << shared_data->leader_pid
                  << ", My PID: " << GET_PID() << std::endl;
    }

    void log_start() {
        std::string message = "Main PID: " + std::to_string(GET_PID()) +
                             ". Timestamp: " + get_current_time();
        log_message(message);
        std::cout << message << std::endl;
    }

    void timer_increment_thread() {
        while (running) {
            SLEEP_MS(300);
            WAIT(semaphore);
            shared_data->counter++;
            POST(semaphore);
        }
    }

    void input_listener_thread() {
        while (running) {
            std::cout << "Enter new counter value: \n";
            int new_value;
            if (std::cin >> new_value) {
                WAIT(semaphore);
                shared_data->counter = new_value;
                POST(semaphore);
                std::cout << "Counter set to: " << new_value << std::endl;
            } else {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
    }

    void logger_thread() {
        if (!is_leader) return;

        while (running) {
            SLEEP_MS(1000);

            WAIT(semaphore);
            int counter_value = shared_data->counter;
            POST(semaphore);

            std::string message = "PID: " + std::to_string(GET_PID()) +
                                ". Timestamp: " + get_current_time() +
                                ". Counter: " + std::to_string(counter_value);
            log_message(message);
            std::cout << message << std::endl;
        }
    }

    void spawner_thread() {
        if (!is_leader) return;

        while (running) {
            SLEEP_MS(3000);

            WAIT(semaphore);
            bool copy1_running = shared_data->copy1_running;
            bool copy2_running = shared_data->copy2_running;
            POST(semaphore);

            if (copy1_running || copy2_running) {
                std::string msg = "Previous copies still running. Skipping spawn.";
                log_message(msg);
                std::cout << msg << std::endl;
                continue;
            }

            std::cout << "Spawning copies..." << std::endl;
            spawn_copy(1);
            spawn_copy(2);
        }
    }

    void spawn_copy(int copy_type) {
        // Флаг запуска
        WAIT(semaphore);
        if (copy_type == 1) {
            shared_data->copy1_running = true;
            shared_data->copy1_pid = 0;
        } else {
            shared_data->copy2_running = true;
            shared_data->copy2_pid = 0;
        }
        POST(semaphore);

        char* args[] = {
            const_cast<char*>("app.exe"),
            const_cast<char*>(copy_type == 1 ? "--type=1" : "--type=2"),
            nullptr
        };

        if (spawn_child_process("app.exe", args)) {
            std::cout << "Spawned Copy_" << copy_type << std::endl;
        } else {
            // Если не удалось запустить, сбрасываем флаг
            WAIT(semaphore);
            if (copy_type == 1) {
                shared_data->copy1_running = false;
            } else {
                shared_data->copy2_running = false;
            }
            POST(semaphore);
            std::cerr << "Failed to spawn Copy_" << copy_type << std::endl;
        }
    }

    void log_message(const std::string& message) {
        LOCK(log_mutex);
        log_file << get_current_time() << " - " << message << std::endl;
        UNLOCK(log_mutex);
    }

    void cleanup() {
        if (log_file.is_open()) log_file.close();
        if (shm_handle) {
            unmap_shared_memory(shm_handle, shared_data);
            close_shared_memory(shm_handle);
        }
        if (semaphore) close_semaphore(semaphore);
        if (log_mutex) close_mutex(log_mutex);
    }
};

Application* global_app = nullptr;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (global_app) global_app->stop();
}

int main(int argc, char* argv[]) {
    // Обработчики сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    bool is_copy = false;
    int copy_type = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--type=1") == 0) {
            is_copy = true;
            copy_type = 1;
            break;
        } else if (strcmp(argv[i], "--type=2") == 0) {
            is_copy = true;
            copy_type = 2;
            break;
        }
    }

    if (is_copy) {
        // Код для копии
        Application copy_app;

        std::string start_msg = "Copy_" + std::to_string(copy_type) +
                               " PID:" + std::to_string(GET_PID()) +
                               ". Timestamp: " + Application::get_current_time();
        std::cout << start_msg << std::endl;

        // Выполняем работу копии
        copy_app.do_copy_work(copy_type);

        // Сбрасываем флаг завершения
        copy_app.mark_copy_finished(copy_type);

        std::string exit_msg = "Copy_" + std::to_string(copy_type) +
                              " Exit timestamp: " + Application::get_current_time();
        std::cout << exit_msg << std::endl;

        return 0;
    } else {
        // Основная программа
        Application app;
        global_app = &app;

        std::cout << "=== Parallel Process Manager ===" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  - Enter any number to set counter value" << std::endl;
        std::cout << "  - Ctrl+C to exit" << std::endl;
        std::cout << "Log file: " << LOG_FILE << std::endl;
        std::cout << "================================" << std::endl;

        app.run();
    }

    std::cout << "Program terminated." << std::endl;
    return 0;
}