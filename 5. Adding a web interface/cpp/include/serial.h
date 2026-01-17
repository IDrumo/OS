#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

class SerialPort {
public:
	enum BaudRate {
		B4800 = 4800,
		B9600 = 9600,
		B19200 = 19200,
		B38400 = 38400,
		B57600 = 57600,
		B115200 = 115200
	};

	SerialPort();
	~SerialPort();

	// Запрещаем копирование
	SerialPort(const SerialPort&) = delete;
	SerialPort& operator=(const SerialPort&) = delete;

	// Разрешаем перемещение
	SerialPort(SerialPort&& other) noexcept;
	SerialPort& operator=(SerialPort&& other) noexcept;

	bool open(const std::string& port, BaudRate baudrate = B115200);
	void close();
	bool is_open() const;

	std::string read_line(char delimiter = '\n');
	void write_string(const std::string& str);

	void set_timeout(double seconds);
	void flush();

private:
#ifdef _WIN32
	HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
	int fd_ = -1;
#endif
	bool is_open_ = false;
};

#endif // SERIAL_H