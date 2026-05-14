#pragma once
// silicon-net: Virtual ASIC Packet Representation
// Minimal Ethernet + IPv4 packet structure for pipeline processing.

#include <array>
#include <cstdint>
#include <cstring>

namespace asic {

using MacAddr = std::array<uint8_t, 6>;

struct EthernetHeader {
    MacAddr dst_mac;
    MacAddr src_mac;
    uint16_t ethertype;  // 0x0800 = IPv4
};

struct Ipv4Header {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t ttl;
    uint8_t protocol;
};

struct Packet {
    EthernetHeader eth;
    Ipv4Header ipv4;
    uint16_t ingress_port;
    uint16_t egress_port;
    bool has_ipv4 = false;
    bool dropped = false;
    // Extensible: add payload pointer/size for real packet data
};

}  // namespace asic
