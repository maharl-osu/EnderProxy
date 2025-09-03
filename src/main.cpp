#include "./networkmanager/networkmanager.hpp"
#include "./packet/packet.hpp"
#include "./config/config.hpp"

#include <iostream>
#include <memory>

// Main thread and entry point
int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Please specify a config file to use as an argument." << std::endl;
        exit(0);
    }

    // Load Config
    Config config(argv[1]);


    // Open Reverse Proxy Port
    NetworkManager::ListenTCP(config.GetPort());
    std::cout << "Config successfully loaded, reverse proxy listening on port " << config.GetPort() << "." << std::endl;

    // Accept Incoming Requests
    while (true) {
        auto connection = NetworkManager::AcceptTCP();

        std::cout << "Received Connection: " << connection->GetIP() << ":" << connection->GetPort() << std::endl;

        Packet packet(connection.get());

        uint32_t packet_id = packet.ReadVarInt();
        uint32_t protocol = packet.ReadVarInt();

        std::cout << "Packet ID: " << packet_id << std::endl;
        std::cout << "Protocol: " << protocol << std::endl;

        auto forward_connection = NetworkManager::ConnectTCP("192.168.0.216:25565");

        packet.Forward(forward_connection.get());
        
        std::string src_addr = connection->GetIP() + ":" + std::to_string(connection->GetPort());
        std::string dst_addr = forward_connection->GetIP() + ":" + std::to_string(forward_connection->GetPort());

        
        NetworkManager::ForwardTCP(src_addr, dst_addr);
        NetworkManager::ForwardTCP(dst_addr, src_addr);
        
        

        connection = nullptr;
        forward_connection = nullptr;
    }

    std::cout << "Main Thread Exiting!" << std::endl;
}