#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
#include <memory>

enum class LINE_TYPE {
    SERVER,   // [example.com]
    PROPERTY, // ip = 1.1.1.1
    EMPTY,    // an empty line
    INVALID   // [example.com
};

enum class PROTOCOL {
    TCP,
    BOTH  
};

// Individual config for a server
struct ServerConfig {
    std::string server_name; // domain or ipv4
    std::string forward_ip; // ipv4
    int port = 25565;
    PROTOCOL protocol;
};

// Overarching config
class Config {
public:
    Config(const std::string&);
    int GetPort() {return port;};
private:
    std::vector<std::unique_ptr<ServerConfig>> servers;
    int port;
};


#endif