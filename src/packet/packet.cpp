#include "./packet.hpp"

#include <stdexcept>
#include <iostream>

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

// Prototypes
int32_t ReadVarIntRaw(TCPConnection*, uint8_t*, ssize_t&);

Packet::Packet(TCPConnection* conn) {

    // Read off length of packet
    buffer_length = 0; // includes offset for length, since we need to forward whole packet
    ssize_t total_read_len = 0; // PacketID + Data length, according to spec
    ssize_t read_len = 0;

    packet_length = ReadVarIntRaw(conn, raw_packet, buffer_length);
    offset = buffer_length; // start processing read commands at the beginning of the actual packet.

    if (packet_length != 0) {
        // Read rest of packet into packet buffer
        while (total_read_len < packet_length && conn->Status() == TCPStatus::OPEN) {
            int amt_to_read = std::min(PACKET_MAX - buffer_length, packet_length - total_read_len);
            conn->Recv(raw_packet + buffer_length, read_len, amt_to_read);
            total_read_len += read_len;
        }
    }

    buffer_length += total_read_len;
}

void Packet::Forward(TCPConnection* conn) {
    conn->Send(raw_packet, buffer_length);
}

int32_t Packet::ReadVarInt() {
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

// Helper Implementations

// Buffer should be of length PACKET_MAX
int32_t ReadVarIntRaw(TCPConnection* conn, uint8_t* buffer, ssize_t& len) {

    len = 0;

    int value = 0;
    int position = 0;
    uint8_t current_byte;

    ssize_t num_written = 0;

    while (conn->Status() == TCPStatus::OPEN) {
        conn->Recv(buffer + len, num_written, 1);

        if (num_written < 1) {
            std::cout << "Failed To Read?" << std::endl;
            break;
        }

        current_byte = buffer[len];
        len += num_written;
        
        value |= (current_byte & SEGMENT_BITS) << position;

        if ((current_byte & CONTINUE_BIT) == 0) break;

        position += 7;

        if (position >= 32) throw std::runtime_error("VarInt is too big");
    }

    return value;
}