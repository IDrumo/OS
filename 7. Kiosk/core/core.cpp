#include "core.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

SerialPort::SerialPort(const std::string& portName) {
    name = portName;
#ifdef _WIN32
    hSerial = CreateFileA(name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial != INVALID_HANDLE_VALUE) {
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (GetCommState(hSerial, &dcbSerialParams)) {
            dcbSerialParams.BaudRate = CBR_9600;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;
            SetCommState(hSerial, &dcbSerialParams);
        }
    }
#else
    fd = open(name.c_str(), O_RDONLY | O_NOCTTY);
#endif
}

SerialPort::~SerialPort() {
#ifdef _WIN32
    if (hSerial != INVALID_HANDLE_VALUE) CloseHandle(hSerial);
#else
    if (fd != -1) close(fd);
#endif
}

bool SerialPort::isOpen() const {
#ifdef _WIN32
    return hSerial != INVALID_HANDLE_VALUE;
#else
    return fd != -1;
#endif
}

bool SerialPort::readLine(std::string& line) {
    line.clear();
    char c;
    while (true) {
#ifdef _WIN32
        DWORD bytesRead;
        if (ReadFile(hSerial, &c, 1, &bytesRead, NULL) && bytesRead > 0) {
            if (c == '\n') return true;
            if (c != '\r') line += c;
        } else return false;
#else
        if (read(fd, &c, 1) > 0) {
            if (c == '\n') return true;
            if (c != '\r') line += c;
        } else return false;
#endif
    }
}

void Simulator::run() {
    std::ofstream out("virtual_com", std::ios::trunc);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(20.0, 30.0);
    while (true) {
        double temp = dist(rng);
        out << temp << "\n";
        out.flush();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}