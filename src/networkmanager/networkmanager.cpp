#include "./networkmanager.hpp"

extern "C" {
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <errno.h>
}

#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <thread>

// TCPConnection Implementation
TCPConnection::TCPConnection(const std::string& ip_port) {
    status = TCPStatus::SETUP;

    // Extract ip and port separately
    size_t pos = ip_port.find(':');

    ip = ip_port.substr(0, pos);
    port = std::atoi(ip_port.substr(pos + 1).c_str());

    // Create socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        throw std::runtime_error("Failed To Create Socket");
    
    sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_in.sin_addr.s_addr);

    if (connect(fd, (sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
        close(fd);
        throw std::runtime_error("Failed To Connect Socket");
    }

    status = TCPStatus::OPEN;
}

TCPConnection::TCPConnection(const size_t& fd, const std::string& ip, const int& port) {
    this->fd = fd;
    this->ip = ip;
    this->port = port;
    
    status = TCPStatus::OPEN;
}

TCPConnection::~TCPConnection() {

    if (status != TCPStatus::CLOSED) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

}

void TCPConnection::Send(uint8_t* buffer, const ssize_t& len) {

    ssize_t len_written = write(fd, buffer, len);

    if (len_written == -1) {
        std::cout << "Failed To Write To TCP Connection" << std::endl;
        Close();
    } if (len_written != len) {
        std::cout << "Failed To Write Entire Packet. Trying To Write Remaining." << std::endl;
        Send(buffer + len_written, len - len_written);
    }

}

void TCPConnection::Recv(uint8_t* recv_buffer, ssize_t& recv_len, const size_t& buffer_size) {

    recv_len = recv(fd, recv_buffer, buffer_size, 0);

    if (recv_len <= 0) { 
        Close();
    } // Error or socket is closed
        
}

void TCPConnection::Close() {
    status = TCPStatus::CLOSED;

    shutdown(fd, SHUT_RDWR);
    close(fd);

    NetworkManager::RemoveConnection(this);
}


// NetworkManager Helper Prototypes
void TCPForwardWorker(std::shared_ptr<TCPConnection>, std::shared_ptr<TCPConnection>);

// NetworkManager Implementation
bool NetworkManager::listening = false;
int NetworkManager::server_fd = -1;
std::vector< std::shared_ptr<TCPConnection> > NetworkManager::tcp_connections;

void NetworkManager::ListenTCP(int port) {

    if (listening == true) {
        std::cout << "ListenTCP called when server is already running." << std::endl;
        return;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        throw std::runtime_error("Failed To Create Socket.");

    sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(port);

    if (bind(fd, (const sockaddr*)&addr_in, sizeof(addr_in)) == -1)
        throw std::runtime_error("Failed To Bind Socket.");

    if (listen(fd, 5) == -1)
        throw std::runtime_error("Failed To Listen on Socket.");

    std::cout << "Listening on Port: " << port << std::endl;

    server_fd = fd;
    listening = true;
}

std::shared_ptr<TCPConnection> NetworkManager::AcceptTCP() {
    if (server_fd == -1) {
        std::cout << "Can't Accept TCP connections before listening." << std::endl;
        return nullptr;
    }

    sockaddr_in addr_in;
    socklen_t len = sizeof(addr_in);

    int fd = accept(server_fd, (sockaddr*)&addr_in, &len);

    char ip_c_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr_in.sin_addr, ip_c_str, INET_ADDRSTRLEN) == NULL) {
        std::cout << "Failed to convert connection ip address to string. Closing connection." << std::endl;
        close(fd);
        return nullptr;
    }

    std::shared_ptr<TCPConnection> tcp_conn = std::make_shared<TCPConnection>(
        fd,
        std::string(ip_c_str),
        addr_in.sin_port
    );

    tcp_connections.push_back(tcp_conn);

    return tcp_conn;
}

std::shared_ptr<TCPConnection> NetworkManager::ConnectTCP(std::string ip_port) {
    
    std::shared_ptr<TCPConnection> connection = std::make_shared<TCPConnection>(ip_port);

    tcp_connections.push_back(connection);

    return connection;
}

bool NetworkManager::ForwardTCP(std::string ip_port_src, std::string ip_port_dst) {

    std::string src_ip, dst_ip;
    int src_port, dst_port;
    
    {
        int pos = ip_port_src.find(':');
        src_ip = ip_port_src.substr(0, pos);
        src_port = std::atoi(ip_port_src.substr(pos + 1).c_str());

        pos = ip_port_dst.find(':');
        dst_ip = ip_port_dst.substr(0, pos);
        dst_port = std::atoi(ip_port_dst.substr(pos + 1).c_str());
    }

    std::shared_ptr<TCPConnection> src_connection = nullptr;
    std::shared_ptr<TCPConnection> dst_connection = nullptr;

    for (auto begin = tcp_connections.begin(); begin != tcp_connections.end(); ++begin) {
        if ( (*begin)->GetIP() == src_ip && (*begin)->GetPort() == src_port) {
            src_connection = *begin;
        } else if ( (*begin)->GetIP() == dst_ip && (*begin)->GetPort() == dst_port)  {
            dst_connection = *begin;
        }
    }

    if (src_connection == nullptr || dst_connection == nullptr) {
        std::cout << "Couldn't forward packets from: " << ip_port_src << " to " << ip_port_dst << std::endl;
        return false;
    }

    std::thread t(TCPForwardWorker, src_connection, dst_connection);
    t.detach();
    std::cout << "Thread created!" << std::endl;

    

    return true;
}

void NetworkManager::RemoveConnection(TCPConnection* connection) {
    for (auto begin = tcp_connections.begin(); begin != tcp_connections.end(); ++begin) {
        if (begin->get() == connection) {
            tcp_connections.erase(begin);
            return;
        }
    }
}


// NetworkManager Helper Implementation
void TCPForwardWorker(std::shared_ptr<TCPConnection> src, std::shared_ptr<TCPConnection> dst) {
    const size_t buffer_size = TCP_BUFFER_SIZE;
    uint8_t buffer[buffer_size];
    ssize_t len;
    
    while (src->Status() == TCPStatus::OPEN && dst->Status() == TCPStatus::OPEN) {
        src->Recv(buffer, len, buffer_size);

        if (len > 0 && dst->Status() == TCPStatus::OPEN)
            dst->Send(buffer, len);
    }

    std::cout << "Forward Worker Exiting." << std::endl;
}