// silicon-net: Silicon SDK Implementation
// TODO: Implement in Milestone 2

#include "silicon_sdk.h"
#include "pipeline.h"
#include <memory>

namespace silicon_sdk {

static std::unique_ptr<asic::Pipeline> g_pipeline;
static bool g_initialized = false;
static NexthopId g_next_nhop_id = 1;
static AclRuleId g_next_acl_id = 1;

Status initialize(const SdkConfig* config) {
    if (g_initialized) return Status::ERR_ALREADY_INITIALIZED;
    g_pipeline = std::make_unique<asic::Pipeline>();
    g_initialized = true;
    return Status::OK;
}

Status shutdown() {
    if (!g_initialized) return Status::ERR_NOT_INITIALIZED;
    g_pipeline.reset();
    g_initialized = false;
    g_next_nhop_id = 1;
    g_next_acl_id = 1;
    return Status::OK;
}

bool is_initialized() { return g_initialized; }

// TODO: Implement all operations in Milestone 2
// Each function should:
// 1. Check g_initialized
// 2. Validate parameters
// 3. Delegate to g_pipeline->tables()
// 4. Return appropriate Status

Status port_set_admin_state(PortId port, bool up) {
    if (!g_initialized) return Status::ERR_NOT_INITIALIZED;
    if (!g_pipeline->ports().set_admin_state(port, up))
        return Status::ERR_INVALID_PARAM;
    return Status::OK;
}

Status port_get_admin_state(PortId port, bool* out) {
    if (!g_initialized) return Status::ERR_NOT_INITIALIZED;
    if (!out) return Status::ERR_INVALID_PARAM;
    auto* p = g_pipeline->ports().get_port(port);
    if (!p) return Status::ERR_INVALID_PARAM;
    *out = p->admin_up;
    return Status::OK;
}

Status port_get_counters(PortId port, PortCounters* out) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status fdb_add_entry(const FdbEntry* entry) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status fdb_remove_entry(const MacAddr& mac, VlanId vlan) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status fdb_get_entry(const MacAddr& mac, VlanId vlan, FdbEntry* out) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status route_add(const RouteEntry* entry) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status route_remove(uint32_t prefix, uint8_t prefix_len) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status nexthop_create(const NexthopEntry* entry, NexthopId* out_id) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status nexthop_remove(NexthopId id) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status acl_add_rule(const AclRule* rule, AclRuleId* out_id) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status acl_remove_rule(AclRuleId id) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

Status inject_packet(const PacketDesc* desc, PacketResult* result) {
    // TODO
    return Status::ERR_NOT_INITIALIZED;
}

}  // namespace silicon_sdk
