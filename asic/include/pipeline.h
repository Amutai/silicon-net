#pragma once
// silicon-net: Virtual ASIC Packet Processing Pipeline

#include "packet.h"
#include "port.h"
#include "tables.h"

namespace asic {

// Pipeline stage results for tracing/debugging
struct PipelineTrace {
    bool fdb_hit = false;
    uint16_t fdb_port = 0;
    bool lpm_hit = false;
    uint32_t lpm_nexthop_id = 0;
    bool acl_matched = false;
    AclAction acl_action = AclAction::PERMIT;
    bool nexthop_resolved = false;
    uint16_t egress_port = 0;
    bool forwarded = false;
    const char* drop_reason = nullptr;
};

class Pipeline {
public:
    Pipeline();

    // Process a packet through all stages. Returns trace for debugging.
    PipelineTrace process(Packet& pkt);

    // Access tables for programming
    PortTable& ports() { return ports_; }
    FdbTable& fdb() { return fdb_; }
    LpmTable& lpm() { return lpm_; }
    NexthopTable& nexthops() { return nexthops_; }
    AclTable& acl() { return acl_; }

private:
    PortTable ports_;
    FdbTable fdb_;
    LpmTable lpm_;
    NexthopTable nexthops_;
    AclTable acl_;

    void stage_ingress(Packet& pkt, PipelineTrace& trace);
    void stage_fdb(Packet& pkt, PipelineTrace& trace);
    void stage_lpm(Packet& pkt, PipelineTrace& trace);
    void stage_acl(Packet& pkt, PipelineTrace& trace);
    void stage_nexthop(Packet& pkt, PipelineTrace& trace);
    void stage_egress(Packet& pkt, PipelineTrace& trace);
};

}  // namespace asic
