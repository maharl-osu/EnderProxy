#include "./packet.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

// Prototypes
int32_t ReadVarIntRaw(TCPConnection*, uint8_t*, ssize_t&);
void WriteVarIntRaw(int value,uint8_t* buffer, int& bytes_used);

Packet::Packet(TCPConnection* conn) {
    packet_type = PacketType::SERVERBOUND;

    // Read off length of packet
    offset = 0;
    ssize_t total_read_len = 0; // PacketID + Data length, according to spec
    ssize_t read_len = 0;

    packet_length = ReadVarIntRaw(conn, raw_packet, offset);
    buffer_length = offset; // re-align buffer length with the number of bytes which were put in the buffer

    if (packet_length != 0 && packet_length != 0xFE) {
        // Read rest of packet into packet buffer
        while (total_read_len < packet_length && conn->Status() == TCPStatus::OPEN) {
            int amt_to_read = std::min(PACKET_MAX - buffer_length, packet_length - total_read_len);
            conn->Recv(raw_packet + buffer_length, read_len, amt_to_read);

            if (read_len > 0) {
                total_read_len += read_len;
                buffer_length += read_len;
            }
                
        }
    }
    
}

Packet::Packet() {
    packet_type = PacketType::CLIENTBOUND;
    packet_length = 0;
    buffer_length = 0;
    offset = 0;
}

// NOTE: THIS FUNCTION WILL NEED TO BE ADJUSTED
// IF THE NEED ARISES TO FORWARD A CLIENTBOUND
// PACKET MORE THAN ONCE.
void Packet::Forward(TCPConnection* conn) {
    if (packet_type == PacketType::SERVERBOUND) {
        // packet is ready to be sent, raw_packet contains
        // every byte on serverbound packets
        conn->Send(raw_packet, buffer_length);
    } else {
        // clientbound packets need to be prefixed with the
        // length
        uint8_t length[5]; // according to the protocol, varint never exceeds 5 bytes.
        int bytes_used; // number of bytes used to represent the length
        WriteVarIntRaw(packet_length, length, bytes_used);

        // Format the packet properly
        std::memmove(raw_packet + bytes_used, raw_packet, packet_length);
        std::memcpy(raw_packet, length, bytes_used);
        buffer_length += bytes_used;

        conn->Send(raw_packet, buffer_length);
    }
    
}

int32_t Packet::ReadVarInt() {
    if (packet_type == PacketType::CLIENTBOUND)
        throw std::runtime_error("Attempting To Read From Clientbound Packet");

    int32_t value = 0;
    int position = 0;
    uint8_t current_byte;

    while (true) {
        current_byte = raw_packet[offset];
        ++offset;
        value |= (current_byte & SEGMENT_BITS) << position;

        if ((current_byte & CONTINUE_BIT) == 0) break;

        position += 7;

        if (position >= 32) throw std::runtime_error("VarInt is too big");
    }

    return value;
}

void Packet::WriteVarInt(int value) {
    while (true) {
        if ((value & ~SEGMENT_BITS) == 0) {
            raw_packet[offset] = value;
            offset++;
            packet_length++;
            buffer_length++;
            return;
        }

        raw_packet[offset] = (value & SEGMENT_BITS) | CONTINUE_BIT;
        offset++;
        packet_length++;
        buffer_length++;

        // Note: >>> means that the sign bit is shifted with the rest of the number rather than being left alone
        value >>= 7;
    }
}

int64_t Packet::ReadLong() {
    if (packet_type == PacketType::CLIENTBOUND)
        throw std::runtime_error("Attempting To Read From Clientbound Packet");
    
    int64_t value;

    std::memcpy(&value, raw_packet + offset, 8);
    offset += 8;

    return value;
}

void Packet::WriteLong(int64_t value) {
    std::memcpy(raw_packet + offset, &value, 8);
    packet_length += 8;
    buffer_length += 8;
    offset += 8;
}

uint16_t Packet::ReadUShort() {
    if (packet_type == PacketType::CLIENTBOUND)
        throw std::runtime_error("Attempting To Read From Clientbound Packet");

    uint16_t value;

    std::memcpy(&value, raw_packet + offset, 2);
    offset += 2;
    value = ntohs(value);

    return value;
}

void Packet::WriteString(std::string value) {
    size_t str_len = value.length();
    const char* c_str = value.c_str();

    WriteVarInt(str_len);
    std::memcpy(raw_packet + offset, c_str, str_len);
    offset += str_len;
    packet_length += str_len;
    buffer_length += str_len;
}

std::string Packet::ReadString() {
    if (packet_type == PacketType::CLIENTBOUND)
        throw std::runtime_error("Attempting To Read From Clientbound Packet");

    int length = ReadVarInt();

    char str[length + 1];
    std::memcpy(str, raw_packet + offset, length);
    str[length] = '\0';
    offset += length;

    return std::string(str);
}

// Helper Implementations

// Buffer should be of length 5, which according to the protocol
// is the longest a varint can be.
int32_t ReadVarIntRaw(TCPConnection* conn, uint8_t* buffer, ssize_t& offset) {
    offset = 0;

    int value = 0;
    int position = 0;
    uint8_t current_byte;

    ssize_t num_written = 0;

    while (conn->Status() == TCPStatus::OPEN) {
        conn->Recv(buffer + offset, num_written, 1);

        if (num_written < 1)
            break;

        current_byte = buffer[offset];
        offset += num_written;
        
        value |= (current_byte & SEGMENT_BITS) << position;

        if ((current_byte & CONTINUE_BIT) == 0) break;

        position += 7;

        if (position >= 32) throw std::runtime_error("VarInt is too big");
    }

    return value;
}

void WriteVarIntRaw(int value, uint8_t* buffer, int& bytes_used) {
    bytes_used = 0;

    while (true) {
        if ((value & ~SEGMENT_BITS) == 0) {
            buffer[bytes_used] = value;
            bytes_used++;
            return;
        }

        buffer[bytes_used] = (value & SEGMENT_BITS) | CONTINUE_BIT;
        bytes_used++;

        // Shift the signed bit with it
        value = (int)((unsigned int)value >> 7);
    }
}