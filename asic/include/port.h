#pragma once
// silicon-net: Virtual ASIC Port Model

#include <cstdint>

namespace asic {

constexpr uint16_t MAX_PORTS = 8;

struct PortCounters {
    uint64_t rx_packets = 0;
    uint64_t tx_packets = 0;
    uint64_t rx_bytes = 0;
    uint64_t tx_bytes = 0;
    uint64_t drops = 0;
};

struct Port {
    uint16_t id = 0;
    bool admin_up = false;
    uint32_t speed_mbps = 10000;  // 10G default
    uint16_t mtu = 9216;
    PortCounters counters;
};

class PortTable {
public:
    PortTable();

    bool set_admin_state(uint16_t port_id, bool up);
    bool is_up(uint16_t port_id) const;
    Port* get_port(uint16_t port_id);
    const Port* get_port(uint16_t port_id) const;
    void increment_rx(uint16_t port_id);
    void increment_tx(uint16_t port_id);
    void increment_drop(uint16_t port_id);

private:
    Port ports_[MAX_PORTS];
};

}  // namespace asic
