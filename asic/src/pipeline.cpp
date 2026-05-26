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
    if (!ports_.is_up(pkt.ingress_port)) {
        pkt.dropped = true;
        trace.drop_reason = "ingress port down";
        ports_.increment_drop(pkt.ingress_port);
        return;
    }
    ports_.increment_rx(pkt.ingress_port);
}

void Pipeline::stage_fdb(Packet& pkt, PipelineTrace& trace) {
    auto result = fdb_.lookup(pkt.eth.dst_mac, 1);
    trace.fdb_hit = result.hit;
    if (result.hit) {
        trace.fdb_port = result.port_id;
        pkt.egress_port = result.port_id;
        return;
    }

    if (pkt.eth.ethertype != 0x0800) {
        pkt.dropped = true;
        trace.drop_reason = "no FDB hit and not IPv4";
        return;
    }
    pkt.has_ipv4 = true;
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