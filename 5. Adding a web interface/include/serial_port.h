#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <string>
#include <atomic>

#ifdef _WIN32
    typedef void* HANDLE;
#else
typedef int HANDLE;
#endif

class SerialPort {
private:
    HANDLE hSerial;
    bool is_open;
    std::atomic<bool> stop_flag;

public:
    SerialPort();
    ~SerialPort();

    bool open(const std::string& port_name, int baud_rate = 9600);
    bool read_line(std::string& line, int timeout_ms = 1000);
    void close();
    bool isOpen() const { return is_open; }

    // Запрещаем копирование
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
};

#endif // SERIAL_PORT_H