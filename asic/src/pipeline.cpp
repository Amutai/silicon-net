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
    if (!pkt.has_ipv4) return;

    auto result = lpm_.lookup(pkt.ipv4.dst_ip);
    trace.lpm_hit = result.hit;
    if (result.hit) {
        trace.lpm_nexthop_id = result.nexthop_id;
    } else {
        pkt.dropped = true;
        trace.drop_reason = "no route";
    }
}

void Pipeline::stage_acl(Packet& pkt, PipelineTrace& trace) {
    auto result = acl_.evaluate(pkt.ipv4.src_ip, pkt.ipv4.dst_ip);
    trace.acl_matched = result.matched;
    trace.acl_action = result.action;

    if (!result.matched) return;

    switch (result.action) {
        case AclAction::DENY:
            pkt.dropped = true;
            trace.drop_reason = "ACL deny";
            return;
        case AclAction::REDIRECT:
            pkt.egress_port = result.redirect_port;
            return;
        case AclAction::PERMIT:
            break;
    }
}

void Pipeline::stage_nexthop(Packet& pkt, PipelineTrace& trace) {
    if (!pkt.has_ipv4 || !trace.lpm_hit) return;

    auto nh = nexthops_.lookup(trace.lpm_nexthop_id);
    if (!nh) {
        pkt.dropped = true;
        trace.drop_reason = "nexthop not found";
        return;
    }

    if (pkt.ipv4.ttl <= 1) {
        pkt.dropped = true;
        trace.drop_reason = "TTL expired";
        return;
    }

    pkt.ipv4.ttl--;
    pkt.eth.dst_mac = nh->dst_mac_rewrite;
    pkt.egress_port = nh->egress_port;
    trace.nexthop_resolved = true;
    trace.egress_port = nh->egress_port;
}

void Pipeline::stage_egress(Packet& pkt, PipelineTrace& trace) {
    if (!ports_.is_up(pkt.egress_port)) {
        pkt.dropped = true;
        trace.drop_reason = "egress port down";
        ports_.increment_drop(pkt.egress_port);
        return;
    }

    ports_.increment_tx(pkt.egress_port);
    trace.forwarded = true;
}

}  // namespace asic