#pragma once
#include <string>

class SerialPort {
public:
    SerialPort(const std::string& portName);
    ~SerialPort();
    bool isOpen() const;
    bool readLine(std::string& line);
private:
    std::string name;
#ifdef _WIN32
    void* hSerial;
#else
    int fd;
#endif
};

class Simulator {
public:
    void run();
};