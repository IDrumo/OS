#include "../include/config.h"
#include <stdexcept>
#include <iostream>
#include <string>
#include <cstring>

Config Config::from_command_line(int argc, char** argv) {
    Config config;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [options]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  --baud <rate>     Baud rate (4800, 9600, 19200, 38400, 57600, 115200)" << std::endl;
        std::cerr << "  --simulate         Run in simulation mode (no real port needed)" << std::endl;
        std::cerr << "  --interval <ms>    Simulation interval in milliseconds" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << argv[0] << " COM1" << std::endl;
        std::cerr << "  " << argv[0] << " COM1 --baud 9600" << std::endl;
        std::cerr << "  " << argv[0] << " --simulate --interval 500" << std::endl;
        exit(1);
    }

    // Обработка аргументов
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--simulate") {
            config.port = "SIMULATION";
        } else if (arg == "--baud" && i + 1 < argc) {
            config.baud_rate = std::stoi(argv[++i]);
        } else if (arg == "--interval" && i + 1 < argc) {
            // Для симулятора
            config.buffer_size = std::stoi(argv[++i]);
        } else if (arg[0] != '-') {
            config.port = arg;
        }
    }
    
    return config;
}