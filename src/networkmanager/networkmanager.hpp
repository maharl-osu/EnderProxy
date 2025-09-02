#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

// Networking
extern "C" {
    #include <arpa/inet.h>
    #include <sys/socket.h>
}


// Other
#include <vector>
#include <memory>
#include <string>

#define TCP_BUFFER_SIZE 512000

enum class TCPStatus {
    SETUP,
    OPEN,
    CLOSED
};


class TCPConnection {
public:
    TCPConnection(const std::string&); // String Example: "192.168.0.1:25565"
    TCPConnection(const size_t&, const std::string&, const int&); // fd, ip, port
    void Send(uint8_t*, const ssize_t&) const; // buffer, amt to write
    void Recv(uint8_t*, size_t&, const size_t&) const; // buffer, num written, buffer size
    void Close();
    const int& GetPort() const {return port;};
    const std::string& GetIP() const {return ip;};
    const TCPStatus& Status() const {return status;};
private:
    int fd;
    std::string ip;
    int port;
    TCPStatus status;
};


class NetworkManager {
public:
    static void ListenTCP(int);
    static std::shared_ptr<TCPConnection> AcceptTCP();
    static std::shared_ptr<TCPConnection> ConnectTCP(std::string);
    static bool ForwardTCP(std::string, std::string);
private:
    static bool listening;
    static int server_fd;
    static std::vector< std::shared_ptr<TCPConnection> > tcp_connections;
};




#endif