#include "config.hpp"
#include <fstream>
#include <iostream>

// Helper Prototypes
void StripLine(std::string&);
LINE_TYPE GetLineType(const std::string&);
void ExtractKeyValue(const std::string&, std::string&, std::string&);


Config::Config(const std::string& path) {
    std::ifstream file(path);
    std::string line;

    // Used to indicate if we've grabbed the required arguments
    // for a server config. By default we say we have so that
    // we can create a new ServerConfig when prompted.
    bool got_ip = true;
    bool got_type = true;

    std::string key;
    std::string value;

    std::unique_ptr<ServerConfig> current_config = nullptr;


    while (std::getline(file, line)) {
        StripLine(line);

        switch (GetLineType(line)) {
            case LINE_TYPE::SERVER:

                if (!got_ip || !got_type) {
                    std::cout << "Error in config file. Please include both a ip and type for each server." << std::endl;
                    throw std::runtime_error("Failed To Parse Config.");
                }

                got_ip = false;
                got_type = false;

                if (current_config != nullptr) {
                    this->servers.push_back(std::move(current_config));
                }

                current_config = std::make_unique<ServerConfig>();
                current_config->server_name = line.substr(1, line.length() - 2);

                std::cout << "Server Directory: " << current_config->server_name << std::endl;

                break;
            case LINE_TYPE::PROPERTY:
                ExtractKeyValue(line, key, value);

                if (key == "port" && current_config == nullptr) {
                    this->port = std::atoi(value.c_str());
                } else if (key == "ip" && current_config != nullptr) {
                    current_config->forward_ip = value;
                    got_ip = true;
                } else if (key == "type" && current_config != nullptr) {
                    
                    if (value == "tcp" || value == "TCP") {
                        current_config->protocol = PROTOCOL::TCP;
                    } else if (value == "both" || value == "BOTH") {
                        current_config->protocol = PROTOCOL::BOTH;
                    } else {
                        std::cout << "Error in config file. Failed to parse type: " << value  << std::endl;
                        throw std::runtime_error("Failed To Parse Config.");
                    }

                    got_type = true;
                } else if (key == "port" && current_config != nullptr) {
                    current_config->port = std::atoi(value.c_str());
                } else {
                    std::cout << "Error in config file." << std::endl;
                    throw std::runtime_error("Failed To Parse Config.");
                }

                std::cout << "Set: " << key << " = " << value << std::endl;
                break;
            case LINE_TYPE::EMPTY:
                continue;
            case LINE_TYPE::INVALID:
                std::cout << "Error in config file." << std::endl;
                throw std::runtime_error("Failed To Parse Config.");
                break;
            default:
                throw std::runtime_error("Something Went Wrong.");
        }

    }

    if (current_config == nullptr) {
        std::cout << "Error in config file. Please include at least one server." << std::endl;
        throw std::runtime_error("Failed To Parse Config.");
    }

    if (!got_ip || !got_type) {
        std::cout << "Error in config file. Please include both a ip and type for each server." << std::endl;
        throw std::runtime_error("Failed To Parse Config.");
    }

    this->servers.push_back(std::move(current_config));
}


// Helper Implementations
void StripLine(std::string& line) {
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == ' ') {
            line.erase(i, 1);
            --i; // Fix indexing since we're removing a character
        } else if (line[i] == '#') {
            line.erase(i);
            return;
        }
    }
}

LINE_TYPE GetLineType(const std::string& line) {
    if (line[0] == '[' && line[line.length() - 1] == ']') {
        return LINE_TYPE::SERVER;
    } else if (line.find('=') != std::string::npos) {
        return LINE_TYPE::PROPERTY;
    } else if (line == "") {
        return LINE_TYPE::EMPTY;
    } else {
        return LINE_TYPE::INVALID;
    }
}

void ExtractKeyValue(const std::string& line, std::string& key, std::string& value) {
    size_t pos = line.find('=');

    if (pos == std::string::npos) {
        throw std::runtime_error("Couldn't find equals operator when extracting key value pair.");
    }

    key = line.substr(0, pos);
    value = line.substr(pos + 1);
}