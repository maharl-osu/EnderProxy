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
    try {
        NetworkManager::ListenTCP(config.GetPort());
    } catch (std::exception e) {
        std::cout << "Failed To Listen On Port " << config.GetPort() << std::endl;
        exit(0);
    }
    
    std::cout << "Config successfully loaded, reverse proxy listening on port " << config.GetPort() << "." << std::endl;

    // Accept Incoming Requests
    while (true) {
        auto connection = NetworkManager::AcceptTCP();

        Packet packet(connection.get());

        uint32_t packet_id = packet.ReadVarInt();
        uint32_t protocol = packet.ReadVarInt();
        std::string server_name = packet.ReadString();
        std::string forward_address = config.GetForwardAddress(server_name);

        if (forward_address == "") {
            std::cout << "Received Connection: " << connection->GetIP() << ":" << connection->GetPort()
                  << " (Packet ID: " << packet_id << ") (Protocol Version: " << protocol << ")"
                  << " (Server Name: " << server_name << ") (Forward IP: N/A)"
                  << std::endl;


            connection->Close();
            continue;
        }

        std::cout << "Received Connection: " << connection->GetIP() << ":" << connection->GetPort()
                  << " (Packet ID: " << packet_id << ") (Protocol Version: " << protocol << ")"
                  << " (Server Name: " << server_name << ") (Forward IP: " << forward_address << ")"
                  << std::endl;

        auto forward_connection = NetworkManager::ConnectTCP(forward_address);

        packet.Forward(forward_connection.get());
        
        std::string src_addr = connection->GetIP() + ":" + std::to_string(connection->GetPort());
        
        NetworkManager::ForwardTCP(src_addr, forward_address);
        NetworkManager::ForwardTCP(forward_address, src_addr);
    }

    std::cout << "Main Thread Exiting!" << std::endl;
}