#include "./networkmanager/networkmanager.hpp"
#include "./packet/packet.hpp"
#include "./config/config.hpp"

#include <iostream>
#include <memory>
#include <thread>

// Prototypes
void InvalidHostWorker(std::shared_ptr<TCPConnection>, uint32_t);

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
        packet.ReadUShort(); // Read past the server port
        int intent = packet.ReadVarInt();
        std::string forward_address = config.GetForwardAddress(server_name);

        std::cout << "Received Connection: " << connection->GetIP() << ":" << connection->GetPort()
                  << " (Packet ID: " << packet_id << ") (Protocol Version: " << protocol << ")"
                  << " (Server Name: " << server_name << ") (Forward IP: " << forward_address
                  << ") (Intent: " << intent << ")"
                  << std::endl;

        if (forward_address == "N/A") {

            if (intent == 1) { // Emulate status to provide feedback
                std::thread t(InvalidHostWorker, connection, protocol);
                t.detach();
            } else { // Disconnect, don't care
                connection->Close();
            }
            
            continue;
        }

        auto forward_connection = NetworkManager::ConnectTCP(forward_address);

        packet.Forward(forward_connection.get());
        
        std::string src_addr = connection->GetIP() + ":" + std::to_string(connection->GetPort());
        
        NetworkManager::ForwardTCP(src_addr, forward_address);
        NetworkManager::ForwardTCP(forward_address, src_addr);
    }

    std::cout << "Main Thread Exiting!" << std::endl;
}

// Implementations
void InvalidHostWorker(std::shared_ptr<TCPConnection> connection, uint32_t protocol) {
    // TODO: Emulate basic server packets to provide debug information
    // for trying to connect to a invalid host through the proxy.

    std::string status_str = "{\"version\": {\"name\": \"EnderProxy\", \"protocol\":" + std::to_string(protocol) + "}, \"description\": {\"text\": \"EnderProxy | This Route Has Not Been Setup\"}}";

    while (connection->Status() == TCPStatus::OPEN) {
        Packet request(connection.get());

        if (connection->Status() != TCPStatus::OPEN)
            break; // Connection was closed while trying to read the packet.

        int packet_id = request.ReadVarInt();

        Packet response;

        switch (packet_id) {
            case 0x00: // Status Request
                response.WriteVarInt(0x00); // Packet ID
                response.WriteString(status_str);
                response.Forward(connection.get());
                break;
            case 0x01: // Ping Request
                response.WriteVarInt(0x01);
                response.WriteLong(request.ReadLong());
                response.Forward(connection.get());
                break;
        }
    }
    
    connection->Close();
}