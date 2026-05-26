// silicon-net: Virtual ASIC — Table Implementations
// TODO: Implement in Milestone 1

#include "tables.h"

namespace asic {

// --- FDB Table ---

bool FdbTable::add(const MacAddr& mac, uint16_t vlan, uint16_t port) {
    if (full()) return false;

    for (auto& entry : entries_) {
        if (entry.valid && entry.mac == mac && entry.vlan_id == vlan) {
            entry.port_id = port;
            return true;
        }
    }

    for (auto& entry : entries_) {
        if (!entry.valid) {
            entry = {mac, vlan, port, true};
            count_++;
            return true;
        }
    }
    return false;
}

bool FdbTable::remove(const MacAddr& mac, uint16_t vlan) {
    for (auto& entry : entries_) {
        if (entry.valid && entry.mac == mac && entry.vlan_id == vlan) {
            entry.valid = false;
            count_--;
            return true;
        }
    }
    return false;
}

FdbLookupResult FdbTable::lookup(const MacAddr& mac, uint16_t vlan) const {
    for (const auto& entry : entries_) {
        if (entry.valid && entry.mac == mac && entry.vlan_id == vlan) {
            return {true, entry.port_id};
        }
    }
    return {false, 0};
}

size_t FdbTable::count() const { return count_; }
bool FdbTable::full() const { return count_ >= FDB_TABLE_SIZE; }

// --- LPM Table ---

bool LpmTable::add(uint32_t prefix, uint8_t prefix_len, uint32_t nexthop_id) {
    if (full() || prefix_len > 32) return false;

    uint32_t mask = prefix_len == 0 ? 0 : ~((1U << (32 - prefix_len)) - 1);
    prefix &= mask;

    for (auto& entry : entries_) {
        if (entry.valid && entry.prefix == prefix && entry.prefix_len == prefix_len) {
            entry.nexthop_id = nexthop_id;
            return true;
        }
    }

    for (auto& entry : entries_) {
        if (!entry.valid) {
            entry = {prefix, prefix_len, nexthop_id, true};
            count_++;
            return true;
        }
    }
    return false;
}

bool LpmTable::remove(uint32_t prefix, uint8_t prefix_len) {
    uint32_t mask = prefix_len == 0 ? 0 : ~((1U << (32 - prefix_len)) - 1);
    prefix &= mask;

    for (auto& entry : entries_) {
        if (entry.valid && entry.prefix == prefix && entry.prefix_len == prefix_len) {
            entry.valid = false;
            count_--;
            return true;
        }
    }
    return false;
}

LpmLookupResult LpmTable::lookup(uint32_t dst_ip) const {
    const LpmEntry* best = nullptr;

    for (const auto& entry : entries_) {
        if (!entry.valid) continue;
        uint32_t mask = entry.prefix_len == 0 ? 0 : ~((1U << (32 - entry.prefix_len)) - 1);
        if ((dst_ip & mask) == entry.prefix) {
            if (!best || entry.prefix_len > best->prefix_len)
                best = &entry;
        }
    }

    if (best) return {true, best->nexthop_id};
    return {false, 0};
}

size_t LpmTable::count() const { return count_; }
bool LpmTable::full() const { return count_ >= LPM_TABLE_SIZE; }

// --- Next-Hop Table ---

bool NexthopTable::add(uint32_t id, uint16_t egress_port, const MacAddr& dst_mac) {
    if (full()) return false;

    for (auto& entry : entries_) {
        if (entry.valid && entry.id == id) {
            entry.egress_port = egress_port;
            entry.dst_mac_rewrite = dst_mac;
            return true;
        }
    }

    for (auto& entry : entries_) {
        if (!entry.valid) {
            entry.id = id;
            entry.egress_port = egress_port;
            entry.dst_mac_rewrite = dst_mac;
            entry.ref_count = 0;
            entry.valid = true;
            count_++;
            return true;
        }
    }
    return false;
}

bool NexthopTable::remove(uint32_t id) {
    for (auto& entry : entries_) {
        if (entry.valid && entry.id == id) {
            if (entry.ref_count > 0) return false;
            entry.valid = false;
            count_--;
            return true;
        }
    }
    return false;
}

std::optional<NexthopEntry> NexthopTable::lookup(uint32_t id) const {
    for (const auto& entry : entries_) {
        if (entry.valid && entry.id == id)
            return entry;
    }
    return std::nullopt;
}

bool NexthopTable::increment_ref(uint32_t id) {
    for (auto& entry : entries_) {
        if (entry.valid && entry.id == id) {
            entry.ref_count++;
            return true;
        }
    }
    return false;
}

bool NexthopTable::decrement_ref(uint32_t id) {
    for (auto& entry : entries_) {
        if (entry.valid && entry.id == id && entry.ref_count > 0) {
            entry.ref_count--;
            return true;
        }
    }
    return false;
}

uint32_t NexthopTable::get_ref_count(uint32_t id) const {
    for (const auto& entry : entries_) {
        if (entry.valid && entry.id == id)
            return entry.ref_count;
    }
    return 0;
}
size_t NexthopTable::count() const { return count_; }
bool NexthopTable::full() const { return count_ >= NEXTHOP_TABLE_SIZE; }

// --- ACL Table ---

bool AclTable::add(const AclRule& rule) {
    if (full()) return false;

    for (auto& r : rules_) {
        if (r.valid && r.id == rule.id) {
            r = rule;
            r.valid = true;
            return true;
        }
    }

    for (auto& r : rules_) {
        if (!r.valid) {
            r = rule;
            r.valid = true;
            count_++;
            return true;
        }
    }
    return false;
}

bool AclTable::remove(uint32_t rule_id) {
    for (auto& rule : rules_) {
        if (rule.valid && rule.id == rule_id) {
            rule.valid = false;
            count_--;
            return true;
        }
    }
    return false;
}

AclResult AclTable::evaluate(uint32_t src_ip, uint32_t dst_ip) const {
    const AclRule* best = nullptr;

    for (const auto& rule : rules_) {
        if (!rule.valid) continue;
        if (rule.src_ip_mask != 0 && (src_ip & rule.src_ip_mask) != rule.src_ip)
            continue;
        if (rule.dst_ip_mask != 0 && (dst_ip & rule.dst_ip_mask) != rule.dst_ip)
            continue;
        if (!best || rule.priority < best->priority)
            best = &rule;
    }

    if (best) return {true, best->action, best->redirect_port};
    return {false, AclAction::PERMIT, 0};
}

size_t AclTable::count() const { return count_; }
bool AclTable::full() const { return count_ >= ACL_TABLE_SIZE; }

}  // namespace asic