// silicon-net: Virtual ASIC — Pipeline Implementation
// TODO: Implement in Milestone 1

#include "pipeline.h"

namespace asic {

Pipeline::Pipeline() = default;

PipelineTrace Pipeline::process(Packet& pkt) {
    PipelineTrace trace{};

    stage_ingress(pkt, trace);
    if (pkt.dropped) return trace;

    stage_fdb(pkt, trace);
    if (pkt.dropped) return trace;

    stage_lpm(pkt, trace);
    if (pkt.dropped) return trace;

    stage_acl(pkt, trace);
    if (pkt.dropped) return trace;

    stage_nexthop(pkt, trace);
    if (pkt.dropped) return trace;

    stage_egress(pkt, trace);
    return trace;
}

void Pipeline::stage_ingress(Packet& pkt, PipelineTrace& trace) {
    // TODO: validate ingress port is up, increment rx counter
}

void Pipeline::stage_fdb(Packet& pkt, PipelineTrace& trace) {
    // TODO: L2 lookup by dst_mac + vlan
}

void Pipeline::stage_lpm(Packet& pkt, PipelineTrace& trace) {
    // TODO: L3 longest-prefix-match on dst_ip (only if IPv4)
}

void Pipeline::stage_acl(Packet& pkt, PipelineTrace& trace) {
    // TODO: evaluate ACL rules against packet fields
}

void Pipeline::stage_nexthop(Packet& pkt, PipelineTrace& trace) {
    // TODO: resolve next-hop, rewrite MAC, decrement TTL
}

void Pipeline::stage_egress(Packet& pkt, PipelineTrace& trace) {
    // TODO: validate egress port is up, increment tx counter
}

}  // namespace asic