#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <deque>
#include <iomanip>
#include <thread>
#include <atomic>
#include <filesystem>
#include <csignal>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <cstdio>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
#endif

using namespace std;
using namespace chrono;

atomic<bool> stop_flag{false};

void setup_signal_handler() {
#ifdef _WIN32
    signal(SIGINT, [](int){ stop_flag = true; });
#else
    struct sigaction sa;
    sa.sa_handler = [](int){ stop_flag = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
#endif
}

string time_to_string(time_t t) {
    tm timeinfo;
#ifdef _WIN32
    gmtime_s(&timeinfo, &t);
#else
    gmtime_r(&t, &timeinfo);
#endif
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return string(buffer);
}

class SerialPort {
#ifdef _WIN32
    HANDLE hSerial = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif
    bool is_open = false;

public:
    bool open(const string& port_name, int baud_rate = 9600) {
#ifdef _WIN32
        hSerial = CreateFileA(port_name.c_str(), GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSerial == INVALID_HANDLE_VALUE) return false;

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            return false;
        }
        dcbSerialParams.BaudRate = baud_rate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbSerialParams)) {
            CloseHandle(hSerial);
            return false;
        }
#else
        fd = ::open(port_name.c_str(), O_RDONLY | O_NOCTTY);
        if (fd == -1) return false;

        termios options;
        tcgetattr(fd, &options);
        cfsetispeed(&options, baud_rate);
        cfsetospeed(&options, baud_rate);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY);
        options.c_oflag &= ~OPOST;
        tcsetattr(fd, TCSANOW, &options);
#endif
        is_open = true;
        return true;
    }

    bool read_line(string& line, int timeout_ms = 1000) {
        if (!is_open) return false;

        line.clear();
        char ch;
        auto start = steady_clock::now();

        while (!stop_flag) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start);
            if (elapsed.count() > timeout_ms) return false;

#ifdef _WIN32
            DWORD bytes_read;
            if (ReadFile(hSerial, &ch, 1, &bytes_read, NULL)) {
                if (bytes_read == 1) {
                    if (ch == '\n' || ch == '\r') {
                        if (!line.empty()) return true;
                    } else {
                        line += ch;
                    }
                }
            }
#else
            int n = ::read(fd, &ch, 1);
            if (n == 1) {
                if (ch == '\n' || ch == '\r') {
                    if (!line.empty()) return true;
                } else {
                    line += ch;
                }
            } else if (n == 0) {
                this_thread::sleep_for(milliseconds(10));
            }
#endif
        }
        return false;
    }

    void close() {
        if (is_open) {
#ifdef _WIN32
            CloseHandle(hSerial);
#else
            ::close(fd);
#endif
            is_open = false;
        }
    }

    ~SerialPort() { close(); }
};

bool read_from_stdin(double& temp, time_t& timestamp, string& time_str) {
    string line;
    if (!getline(cin, line)) return false;

    size_t comma = line.find(',');
    if (comma == string::npos) return false;

    time_str = line.substr(0, comma);
    string temp_str = line.substr(comma + 1);

    tm tm_struct = {};
    if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%dZ",
               &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
               &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) != 6) return false;

    tm_struct.tm_year -= 1900;
    tm_struct.tm_mon -= 1;
    tm_struct.tm_isdst = 0;

#ifdef _WIN32
    timestamp = _mkgmtime(&tm_struct);
#else
    timestamp = timegm(&tm_struct);
#endif
    temp = stod(temp_str);
    return true;
}

bool read_from_port(SerialPort& port, double& temp, time_t& timestamp, string& time_str) {
    string line;
    if (!port.read_line(line)) return false;

    size_t comma = line.find(',');
    if (comma != string::npos) {
        time_str = line.substr(0, comma);
        string temp_str = line.substr(comma + 1);

        tm tm_struct = {};
        if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%dZ",
                   &tm_struct.tm_year, &tm_struct.tm_mon, &tm_struct.tm_mday,
                   &tm_struct.tm_hour, &tm_struct.tm_min, &tm_struct.tm_sec) != 6) {
            timestamp = system_clock::to_time_t(system_clock::now());
            time_str = time_to_string(timestamp);
        } else {
            tm_struct.tm_year -= 1900;
            tm_struct.tm_mon -= 1;
            tm_struct.tm_isdst = 0;
#ifdef _WIN32
            timestamp = _mkgmtime(&tm_struct);
#else
            timestamp = timegm(&tm_struct);
#endif
        }
        temp = stod(temp_str);
    } else {
        timestamp = system_clock::to_time_t(system_clock::now());
        time_str = time_to_string(timestamp);
        temp = stod(line);
    }

    return true;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    SetConsoleOutputCP(CP_UTF8);
#endif

    setup_signal_handler();

    string port_name;
    bool use_port = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port_name = argv[++i];
            use_port = true;
        } else if (arg == "--help") {
            cerr << "Использование:" << endl;
            cerr << "  " << argv[0] << " [--port COMx]" << endl;
            cerr << "  --port COMx    : Использовать последовательный порт" << endl;
            cerr << "                   (COM1, COM2 для Windows; /dev/ttyUSB0, /dev/ttyACM0 для Linux)" << endl;
            cerr << "  Без аргументов : Чтение данных из stdin (для работы с симулятором)" << endl;
            return 0;
        }
    }

    filesystem::create_directories("logs");

    string meas_file = "logs/measurements.log";
    string hour_file = "logs/hourly.log";
    string day_file = "logs/daily.log";

    ofstream meas(meas_file, ios::app);
    ofstream hourly(hour_file, ios::app);
    ofstream daily(day_file, ios::app);

    if (!meas || !hourly || !daily) {
        cerr << "Ошибка открытия файлов логов!" << endl;
        return 1;
    }

    SerialPort serial_port;
    if (use_port) {
        if (!serial_port.open(port_name, 9600)) {
            cerr << "Ошибка: не удалось открыть порт " << port_name << endl;
            return 1;
        }
        cerr << "Используется порт: " << port_name << endl;
    } else {
        cerr << "Режим: чтение из stdin (симулятор)" << endl;
    }

    cerr << "Температурный логгер запущен" << endl;
    cerr << "Файлы логов: " << meas_file << ", " << hour_file << ", " << day_file << endl;

    double hour_sum = 0, day_sum = 0;
    int hour_count = 0, day_count = 0;

    time_t current_hour = 0;
    time_t current_day = 0;

    vector<string> measurement_buffer;
    const int BUFFER_SIZE = 10;

    string time_str;
    double temp;
    time_t timestamp;
    int total_count = 0;

    while (!stop_flag) {
        bool success;

        if (use_port) {
            success = read_from_port(serial_port, temp, timestamp, time_str);
        } else {
            success = read_from_stdin(temp, timestamp, time_str);
        }

        if (!success) {
            this_thread::sleep_for(milliseconds(100));
            continue;
        }

        measurement_buffer.push_back(time_str + "," + to_string(temp));

        if (measurement_buffer.size() >= BUFFER_SIZE) {
            for (const auto& entry : measurement_buffer) {
                meas << entry << "\n";
            }
            meas.flush();
            measurement_buffer.clear();
            cerr << "Записано " << BUFFER_SIZE << " измерений" << endl;
        }

        total_count++;

        if (current_hour == 0) current_hour = (timestamp / 3600) * 3600;
        if (current_day == 0) current_day = (timestamp / 86400) * 86400;

        hour_sum += temp;
        hour_count++;
        day_sum += temp;
        day_count++;

        time_t sample_hour = (timestamp / 3600) * 3600;
        if (sample_hour != current_hour && hour_count > 0) {
            double hour_avg = hour_sum / hour_count;
            hourly << time_to_string(current_hour) << "," << fixed << setprecision(3) << hour_avg << "\n";
            hourly.flush();

            hour_sum = hour_count = 0;
            current_hour = sample_hour;
            cerr << "Среднее за час: " << hour_avg << endl;
        }

        time_t sample_day = (timestamp / 86400) * 86400;
        if (sample_day != current_day && day_count > 0) {
            double day_avg = day_sum / day_count;
            daily << time_to_string(current_day) << "," << fixed << setprecision(3) << day_avg << "\n";
            daily.flush();

            day_sum = day_count = 0;
            current_day = sample_day;
            cerr << "Среднее за день: " << day_avg << endl;
        }
    }

    if (!measurement_buffer.empty()) {
        for (const auto& entry : measurement_buffer) {
            meas << entry << "\n";
        }
    }

    if (hour_count > 0) {
        double hour_avg = hour_sum / hour_count;
        hourly << time_to_string(current_hour) << "," << fixed << setprecision(3) << hour_avg << "\n";
    }

    if (day_count > 0) {
        double day_avg = day_sum / day_count;
        daily << time_to_string(current_day) << "," << fixed << setprecision(3) << day_avg << "\n";
    }

    cerr << "Программа завершена. Всего измерений: " << total_count << endl;
    return 0;
}