#pragma once
#include <string>
#include "database.h"

class WebServer {
public:
    WebServer(int port);
    void start(Database& db);
private:
    int port;
};