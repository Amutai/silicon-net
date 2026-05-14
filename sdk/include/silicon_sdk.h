#pragma once
// silicon-net: Silicon SDK — Public API
// Programs the virtual ASIC through a managed interface with resource tracking.

#include <cstdint>
#include <array>

namespace silicon_sdk {

// --- Status Codes ---
enum class Status {
    OK = 0,
    ERR_NOT_INITIALIZED,
    ERR_ALREADY_INITIALIZED,
    ERR_INVALID_PARAM,
    ERR_TABLE_FULL,
    ERR_NOT_FOUND,
    ERR_EXISTS,
    ERR_IN_USE,          // Object has active references
    ERR_PORT_DOWN,
    ERR_INTERNAL,
};

// --- Types ---
using MacAddr = std::array<uint8_t, 6>;
using PortId = uint16_t;
using VlanId = uint16_t;
using NexthopId = uint32_t;
using AclRuleId = uint32_t;

struct SdkConfig {
    uint16_t num_ports = 8;
    // Extensible: add table size overrides, logging config, etc.
};

struct PortCounters {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t drops;
};

struct FdbEntry {
    MacAddr mac;
    VlanId vlan;
    PortId port;
};

struct RouteEntry {
    uint32_t prefix;      // Host byte order
    uint8_t prefix_len;
    NexthopId nexthop_id;
};

struct NexthopEntry {
    uint32_t ip;          // Next-hop IP (for display/debug)
    PortId egress_port;
    MacAddr dst_mac;
};

enum class AclAction : uint8_t {
    PERMIT,
    DENY,
    REDIRECT
};

struct AclRule {
    uint32_t priority;
    uint32_t src_ip;
    uint32_t src_ip_mask;
    uint32_t dst_ip;
    uint32_t dst_ip_mask;
    AclAction action;
    PortId redirect_port;  // Only for REDIRECT
};

// --- Packet injection (testing/debug) ---
struct PacketDesc {
    PortId ingress_port;
    MacAddr dst_mac;
    MacAddr src_mac;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t ttl;
};

struct PacketResult {
    bool forwarded;
    PortId egress_port;
    const char* drop_reason;
};

// --- SDK API ---

Status initialize(const SdkConfig* config);
Status shutdown();
bool is_initialized();

// Port operations
Status port_set_admin_state(PortId port, bool up);
Status port_get_admin_state(PortId port, bool* out);
Status port_get_counters(PortId port, PortCounters* out);

// FDB operations
Status fdb_add_entry(const FdbEntry* entry);
Status fdb_remove_entry(const MacAddr& mac, VlanId vlan);
Status fdb_get_entry(const MacAddr& mac, VlanId vlan, FdbEntry* out);

// Route operations
Status route_add(const RouteEntry* entry);
Status route_remove(uint32_t prefix, uint8_t prefix_len);

// Next-hop operations
Status nexthop_create(const NexthopEntry* entry, NexthopId* out_id);
Status nexthop_remove(NexthopId id);

// ACL operations
Status acl_add_rule(const AclRule* rule, AclRuleId* out_id);
Status acl_remove_rule(AclRuleId id);

// Packet injection (for testing full pipeline)
Status inject_packet(const PacketDesc* desc, PacketResult* result);

}  // namespace silicon_sdk
