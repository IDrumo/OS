#include "database.h"
#include "web.h"
#include "core.h"
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    std::string port_name = argc > 1 ? argv[1] : "COM3";
#else
    std::string port_name = argc > 1 ? argv[1] : "virtual_com";
#endif
    
    Database db("weather.db");
    WebServer server(8080);
    std::thread server_thread([&]() { server.start(db); });
    
    SerialPort sp(port_name);
    if (!sp.isOpen()) {
        std::cerr << "Failed to open port" << std::endl;
        return 1;
    }
    
    while (true) {
        std::string data;
        if (sp.readLine(data) && !data.empty()) {
            try {
                double temp = std::stod(data);
                db.insert_temp(temp);
            } catch (...) {}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (server_thread.joinable()) server_thread.join();
    return 0;
}