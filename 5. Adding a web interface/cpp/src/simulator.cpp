#include "../include/serial.h"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <atomic>
#include <cstdlib>  // для std::rand

std::atomic<bool> running(true);

void signal_handler(int) {
    running = false;
}

std::string calculate_checksum(const std::string& data) {
    unsigned char sum = 0;
    for (char c : data) {
        sum ^= static_cast<unsigned char>(c);
    }

    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(sum);
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <port> [interval_ms=1000]" << std::endl;
        return 1;
    }

    int interval_ms = 1000;
    if (argc > 2) {
        interval_ms = std::stoi(argv[2]);
    }

    std::signal(SIGINT, signal_handler);

    try {
        SerialPort port;

        if (!port.open(argv[1], SerialPort::B115200)) {
            std::cerr << "Failed to open port: " << argv[1] << std::endl;
            return 1;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> temp_dist(22.0, 5.0);

        std::cout << "Simulating temperature sensor on port " << argv[1] << std::endl;
        std::cout << "Interval: " << interval_ms << "ms" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        double last_temp = 22.0;

        while (running) {
            // Generate temperature with some continuity
            double temp = temp_dist(gen);
            temp = 0.7 * temp + 0.3 * last_temp;  // Smooth changes
            last_temp = temp;

            // Clamp to reasonable values
            if (temp < -40.0) temp = -40.0;
            if (temp > 60.0) temp = 60.0;

            std::string temp_str = std::to_string(temp);
            std::string checksum = calculate_checksum(temp_str);

            std::stringstream packet;
            packet << std::fixed << std::setprecision(2) << temp
                   << " " << checksum << "\n";

            port.write_string(packet.str());
            std::cout << "Sent: " << packet.str();

            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }

        port.close();
        std::cout << "\nSimulator stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}