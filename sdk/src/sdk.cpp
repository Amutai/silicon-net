// silicon-net: Silicon SDK Implementation

#include "silicon_sdk.h"
#include "pipeline.h"
#include <memory>
#include <unordered_map>

namespace silicon_sdk {

// --- Internal State ---
static std::unique_ptr<asic::Pipeline> g_pipeline;
static bool g_initialized = false;

// Object ID management (#15)
static NexthopId g_next_nhop_id = 1;
static AclRuleId g_next_acl_id = 1;
static std::unordered_map<NexthopId, bool> g_nhop_ids;
static std::unordered_map<AclRuleId, bool> g_acl_ids;

// --- Helpers ---
static inline Status check_init() {
    return g_initialized ? Status::OK : Status::ERR_NOT_INITIALIZED;
}

// --- Lifecycle (#14) ---

Status initialize(const SdkConfig* config) {
    if (g_initialized) return Status::ERR_ALREADY_INITIALIZED;
    g_pipeline = std::make_unique<asic::Pipeline>();
    g_initialized = true;
    g_next_nhop_id = 1;
    g_next_acl_id = 1;
    g_nhop_ids.clear();
    g_acl_ids.clear();
    return Status::OK;
}

Status shutdown() {
    if (!g_initialized) return Status::ERR_NOT_INITIALIZED;
    g_pipeline.reset();
    g_initialized = false;
    g_nhop_ids.clear();
    g_acl_ids.clear();
    return Status::OK;
}

bool is_initialized() { return g_initialized; }

// --- Port Operations (#17) ---

Status port_set_admin_state(PortId port, bool up) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!g_pipeline->ports().set_admin_state(port, up))
        return Status::ERR_INVALID_PARAM;
    return Status::OK;
}

Status port_get_admin_state(PortId port, bool* out) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!out) return Status::ERR_INVALID_PARAM;
    auto* p = g_pipeline->ports().get_port(port);
    if (!p) return Status::ERR_INVALID_PARAM;
    *out = p->admin_up;
    return Status::OK;
}

Status port_get_counters(PortId port, PortCounters* out) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!out) return Status::ERR_INVALID_PARAM;
    auto* p = g_pipeline->ports().get_port(port);
    if (!p) return Status::ERR_INVALID_PARAM;
    out->rx_packets = p->counters.rx_packets;
    out->tx_packets = p->counters.tx_packets;
    out->rx_bytes = p->counters.rx_bytes;
    out->tx_bytes = p->counters.tx_bytes;
    out->drops = p->counters.drops;
    return Status::OK;
}

// --- FDB Operations (#18) ---

Status fdb_add_entry(const FdbEntry* entry) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!entry) return Status::ERR_INVALID_PARAM;
    if (!g_pipeline->fdb().add(entry->mac, entry->vlan, entry->port))
        return Status::ERR_TABLE_FULL;
    return Status::OK;
}

Status fdb_remove_entry(const MacAddr& mac, VlanId vlan) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!g_pipeline->fdb().remove(mac, vlan))
        return Status::ERR_NOT_FOUND;
    return Status::OK;
}

Status fdb_get_entry(const MacAddr& mac, VlanId vlan, FdbEntry* out) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!out) return Status::ERR_INVALID_PARAM;
    auto result = g_pipeline->fdb().lookup(mac, vlan);
    if (!result.hit) return Status::ERR_NOT_FOUND;
    out->mac = mac;
    out->vlan = vlan;
    out->port = result.port_id;
    return Status::OK;
}

// --- NextHop Operations (#20) ---

Status nexthop_create(const NexthopEntry* entry, NexthopId* out_id) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!entry || !out_id) return Status::ERR_INVALID_PARAM;
    if (g_pipeline->nexthops().full()) return Status::ERR_TABLE_FULL;

    NexthopId id = g_next_nhop_id++;
    if (!g_pipeline->nexthops().add(id, entry->egress_port, entry->dst_mac))
        return Status::ERR_TABLE_FULL;

    g_nhop_ids[id] = true;
    *out_id = id;
    return Status::OK;
}

Status nexthop_remove(NexthopId id) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (g_nhop_ids.find(id) == g_nhop_ids.end())
        return Status::ERR_NOT_FOUND;
    if (g_pipeline->nexthops().get_ref_count(id) > 0)
        return Status::ERR_IN_USE;
    if (!g_pipeline->nexthops().remove(id))
        return Status::ERR_INTERNAL;

    g_nhop_ids.erase(id);
    return Status::OK;
}

// --- Route Operations (#19) ---

Status route_add(const RouteEntry* entry) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!entry) return Status::ERR_INVALID_PARAM;
    if (g_nhop_ids.find(entry->nexthop_id) == g_nhop_ids.end())
        return Status::ERR_NOT_FOUND;

    if (!g_pipeline->lpm().add(entry->prefix, entry->prefix_len, entry->nexthop_id))
        return Status::ERR_TABLE_FULL;

    g_pipeline->nexthops().increment_ref(entry->nexthop_id);
    return Status::OK;
}

Status route_remove(uint32_t prefix, uint8_t prefix_len) {
    auto s = check_init(); if (s != Status::OK) return s;

    // Find the nexthop_id before removing so we can decrement ref
    auto result = g_pipeline->lpm().lookup(prefix);
    if (!result.hit) return Status::ERR_NOT_FOUND;

    uint32_t nhop_id = result.nexthop_id;
    if (!g_pipeline->lpm().remove(prefix, prefix_len))
        return Status::ERR_NOT_FOUND;

    g_pipeline->nexthops().decrement_ref(nhop_id);
    return Status::OK;
}

// --- ACL Operations (#21) ---

Status acl_add_rule(const AclRule* rule, AclRuleId* out_id) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!rule || !out_id) return Status::ERR_INVALID_PARAM;
    if (g_pipeline->acl().full()) return Status::ERR_TABLE_FULL;

    AclRuleId id = g_next_acl_id++;
    asic::AclRule asic_rule{};
    asic_rule.id = id;
    asic_rule.priority = rule->priority;
    asic_rule.src_ip = rule->src_ip;
    asic_rule.src_ip_mask = rule->src_ip_mask;
    asic_rule.dst_ip = rule->dst_ip;
    asic_rule.dst_ip_mask = rule->dst_ip_mask;
    asic_rule.action = static_cast<asic::AclAction>(rule->action);
    asic_rule.redirect_port = rule->redirect_port;

    if (!g_pipeline->acl().add(asic_rule))
        return Status::ERR_TABLE_FULL;

    g_acl_ids[id] = true;
    *out_id = id;
    return Status::OK;
}

Status acl_remove_rule(AclRuleId id) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (g_acl_ids.find(id) == g_acl_ids.end())
        return Status::ERR_NOT_FOUND;
    if (!g_pipeline->acl().remove(id))
        return Status::ERR_INTERNAL;

    g_acl_ids.erase(id);
    return Status::OK;
}

// --- Packet Injection (#22) ---

Status inject_packet(const PacketDesc* desc, PacketResult* result) {
    auto s = check_init(); if (s != Status::OK) return s;
    if (!desc || !result) return Status::ERR_INVALID_PARAM;

    asic::Packet pkt{};
    pkt.ingress_port = desc->ingress_port;
    pkt.eth.dst_mac = desc->dst_mac;
    pkt.eth.src_mac = desc->src_mac;
    pkt.eth.ethertype = 0x0800;
    pkt.ipv4.src_ip = desc->src_ip;
    pkt.ipv4.dst_ip = desc->dst_ip;
    pkt.ipv4.ttl = desc->ttl;

    auto trace = g_pipeline->process(pkt);

    result->forwarded = trace.forwarded;
    result->egress_port = pkt.egress_port;
    result->drop_reason = trace.drop_reason;
    return Status::OK;
}

}  // namespace silicon_sdk
