// silicon-net: Virtual ASIC — Port Model Implementation
// TODO: Implement in Milestone 1

#include "port.h"

namespace asic {

PortTable::PortTable() {
    for (uint16_t i = 0; i < MAX_PORTS; i++) {
        ports_[i].id = i;
    }
}

bool PortTable::set_admin_state(uint16_t port_id, bool up) {
    if (port_id >= MAX_PORTS) return false;
    ports_[port_id].admin_up = up;
    return true;
}

bool PortTable::is_up(uint16_t port_id) const {
    if (port_id >= MAX_PORTS) return false;
    return ports_[port_id].admin_up;
}

Port* PortTable::get_port(uint16_t port_id) {
    if (port_id >= MAX_PORTS) return nullptr;
    return &ports_[port_id];
}

const Port* PortTable::get_port(uint16_t port_id) const {
    if (port_id >= MAX_PORTS) return nullptr;
    return &ports_[port_id];
}

void PortTable::increment_rx(uint16_t port_id) {
    if (port_id < MAX_PORTS) ports_[port_id].counters.rx_packets++;
}

void PortTable::increment_tx(uint16_t port_id) {
    if (port_id < MAX_PORTS) ports_[port_id].counters.tx_packets++;
}

void PortTable::increment_drop(uint16_t port_id) {
    if (port_id < MAX_PORTS) ports_[port_id].counters.drops++;
}

}  // namespace asic
