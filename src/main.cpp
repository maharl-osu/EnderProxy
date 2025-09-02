#include "./networkmanager/networkmanager.hpp"

#include <iostream>

// Main thread and entry point
int main(int argc, char** argv) {
    
    NetworkManager::ListenTCP(25566);

    for(;;) {
        auto connection = NetworkManager::AcceptTCP();

        std::cout << "Received Connection: " << connection->GetIP() << ":" << connection->GetPort() << std::endl;

        auto forward_connection = NetworkManager::ConnectTCP("192.168.0.216:25565");

        std::string src_addr = connection->GetIP() + ":" + std::to_string(connection->GetPort());
        std::string dst_addr = forward_connection->GetIP() + ":" + std::to_string(forward_connection->GetPort());

        NetworkManager::ForwardTCP(src_addr, dst_addr);
        NetworkManager::ForwardTCP(dst_addr, src_addr);
    }


}