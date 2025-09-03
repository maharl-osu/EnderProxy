#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <cstddef>
#include <string>
#include "../networkmanager/networkmanager.hpp"

// Maximum java edition packet size
#define PACKET_MAX 2097152

class Packet {
public:
    Packet(TCPConnection*);
    void Seek(size_t);
    int32_t ReadVarInt();
    std::string ReadString();
    void Forward(TCPConnection*);
private:
    uint8_t raw_packet[2097152]; // 
    size_t offset;
    ssize_t packet_length; // number of bytes for 
    ssize_t buffer_length; // total number of bytes in raw packet
};



#endif