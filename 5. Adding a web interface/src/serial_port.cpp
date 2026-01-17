#include "serial_port.h"
#include <chrono>
#include <thread>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <cstdio>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <cerrno>
#endif

using namespace std::chrono;

SerialPort::SerialPort() : is_open(false), stop_flag(false) {
#ifdef _WIN32
    hSerial = INVALID_HANDLE_VALUE;
#else
    hSerial = -1;
#endif
}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(const std::string& port_name, int baud_rate) {
#ifdef _WIN32
    hSerial = CreateFileA(port_name.c_str(), GENERIC_READ, 0, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Не удалось открыть порт " << port_name << std::endl;
        return false;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState((HANDLE)hSerial, &dcbSerialParams)) {
        CloseHandle((HANDLE)hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState((HANDLE)hSerial, &dcbSerialParams)) {
        CloseHandle((HANDLE)hSerial);
        return false;
    }

    // Настройка таймаутов
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts((HANDLE)hSerial, &timeouts);
#else
    hSerial = ::open(port_name.c_str(), O_RDONLY | O_NOCTTY | O_NDELAY);
    if (hSerial == -1) {
        return false;
    }

    // Устанавливаем блокирующий режим
    int flags = fcntl(hSerial, F_GETFL, 0);
    fcntl(hSerial, F_SETFL, flags & ~O_NDELAY);

    termios options;
    tcgetattr(hSerial, &options);
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

    options.c_cc[VMIN] = 1;   // Минимум 1 символ
    options.c_cc[VTIME] = 10; // Таймаут 1 секунда

    tcsetattr(hSerial, TCSANOW, &options);
#endif

    is_open = true;
    return true;
}

bool SerialPort::read_line(std::string& line, int timeout_ms) {
    if (!is_open) return false;

    line.clear();
    char ch;
    auto start = steady_clock::now();

    while (!stop_flag) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - start);
        if (elapsed.count() > timeout_ms) {
            return false;
        }

#ifdef _WIN32
        DWORD bytes_read;
        if (ReadFile((HANDLE)hSerial, &ch, 1, &bytes_read, NULL)) {
            if (bytes_read == 1) {
                if (ch == '\n' || ch == '\r') {
                    if (!line.empty()) return true;
                } else {
                    line += ch;
                }
            }
        } else {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                return false;
            }
        }
#else
        int n = read(hSerial, &ch, 1);
        if (n == 1) {
            if (ch == '\n' || ch == '\r') {
                if (!line.empty()) return true;
            } else {
                line += ch;
            }
        } else if (n == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
#endif
    }

    return false;
}

void SerialPort::close() {
    if (is_open) {
        stop_flag = true;
#ifdef _WIN32
        CloseHandle((HANDLE)hSerial);
        hSerial = INVALID_HANDLE_VALUE;
#else
        ::close(hSerial);
        hSerial = -1;
#endif
        is_open = false;
    }
}