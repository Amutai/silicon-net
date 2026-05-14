#pragma once
// silicon-net: Virtual ASIC Forwarding Tables

#include <array>
#include <cstdint>
#include <optional>
#include "packet.h"

namespace asic {

// --- FDB Table ---
constexpr size_t FDB_TABLE_SIZE = 1024;

struct FdbEntry {
    MacAddr mac;
    uint16_t vlan_id;
    uint16_t port_id;
    bool valid = false;
};

struct FdbLookupResult {
    bool hit;
    uint16_t port_id;
};

class FdbTable {
public:
    bool add(const MacAddr& mac, uint16_t vlan, uint16_t port);
    bool remove(const MacAddr& mac, uint16_t vlan);
    FdbLookupResult lookup(const MacAddr& mac, uint16_t vlan) const;
    size_t count() const;
    bool full() const;

private:
    std::array<FdbEntry, FDB_TABLE_SIZE> entries_{};
    size_t count_ = 0;
};

// --- LPM Table ---
constexpr size_t LPM_TABLE_SIZE = 512;

struct LpmEntry {
    uint32_t prefix;       // Network address (host byte order)
    uint8_t prefix_len;    // /0 to /32
    uint32_t nexthop_id;
    bool valid = false;
};

struct LpmLookupResult {
    bool hit;
    uint32_t nexthop_id;
};

class LpmTable {
public:
    bool add(uint32_t prefix, uint8_t prefix_len, uint32_t nexthop_id);
    bool remove(uint32_t prefix, uint8_t prefix_len);
    LpmLookupResult lookup(uint32_t dst_ip) const;
    size_t count() const;
    bool full() const;

private:
    std::array<LpmEntry, LPM_TABLE_SIZE> entries_{};
    size_t count_ = 0;
};

// --- Next-Hop Table ---
constexpr size_t NEXTHOP_TABLE_SIZE = 128;

struct NexthopEntry {
    uint32_t id;
    uint16_t egress_port;
    MacAddr dst_mac_rewrite;
    uint32_t ref_count = 0;
    bool valid = false;
};

class NexthopTable {
public:
    bool add(uint32_t id, uint16_t egress_port, const MacAddr& dst_mac);
    bool remove(uint32_t id);
    std::optional<NexthopEntry> lookup(uint32_t id) const;
    bool increment_ref(uint32_t id);
    bool decrement_ref(uint32_t id);
    uint32_t get_ref_count(uint32_t id) const;
    size_t count() const;
    bool full() const;

private:
    std::array<NexthopEntry, NEXTHOP_TABLE_SIZE> entries_{};
    size_t count_ = 0;
};

// --- ACL Table ---
constexpr size_t ACL_TABLE_SIZE = 256;

enum class AclAction : uint8_t {
    PERMIT,
    DENY,
    REDIRECT
};

struct AclRule {
    uint32_t id;
    uint32_t priority;       // Lower = higher priority
    // Match fields (0 = wildcard)
    uint32_t src_ip = 0;
    uint32_t src_ip_mask = 0;
    uint32_t dst_ip = 0;
    uint32_t dst_ip_mask = 0;
    uint16_t src_port = 0;   // L4 (future)
    uint16_t dst_port = 0;   // L4 (future)
    // Action
    AclAction action = AclAction::PERMIT;
    uint16_t redirect_port = 0;  // Only if action == REDIRECT
    bool valid = false;
};

struct AclResult {
    bool matched;
    AclAction action;
    uint16_t redirect_port;
};

class AclTable {
public:
    bool add(const AclRule& rule);
    bool remove(uint32_t rule_id);
    AclResult evaluate(uint32_t src_ip, uint32_t dst_ip) const;
    size_t count() const;
    bool full() const;

private:
    std::array<AclRule, ACL_TABLE_SIZE> rules_{};
    size_t count_ = 0;
};

}  // namespace asic
