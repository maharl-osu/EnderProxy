#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <cstddef>
#include <string>
#include "../networkmanager/networkmanager.hpp"

// Maximum java edition packet size
#define PACKET_MAX 2097152

enum class PacketType {
    SERVERBOUND,
    CLIENTBOUND
};

class Packet {
public:
    Packet(TCPConnection*);
    Packet();
    void Seek(size_t);
    int32_t ReadVarInt();
    void WriteVarInt(int);
    int64_t ReadLong();
    void WriteLong(int64_t);
    uint16_t ReadUShort();
    void WriteString(std::string);
    std::string ReadString();
    void Forward(TCPConnection*);
private:
    PacketType packet_type;
    uint8_t raw_packet[2097152]; // For serverbound, includes every byte. For clientbound, excludes length, will be prepended on forward
    ssize_t offset;
    ssize_t packet_length; // number of bytes for 
    ssize_t buffer_length; // total number of bytes in raw packet
};



#endif