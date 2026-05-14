// silicon-net: Virtual ASIC Pipeline Tests
// TODO: Implement in Milestone 1

#include <gtest/gtest.h>
#include "pipeline.h"

namespace asic::test {

TEST(Pipeline, ForwardL3Packet) {
    Pipeline pipeline;

    // Setup: port 0 up, port 3 up
    pipeline.ports().set_admin_state(0, true);
    pipeline.ports().set_admin_state(3, true);

    // Add next-hop: id=1, egress port 3
    MacAddr nh_mac = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0x02};
    pipeline.nexthops().add(1, 3, nh_mac);

    // Add route: 10.0.2.0/24 → nhop 1
    pipeline.lpm().add(0x0A000200, 24, 1);

    // Create packet: dst_ip = 10.0.2.20
    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.has_ipv4 = true;
    pkt.ipv4.dst_ip = 0x0A000214;  // 10.0.2.20
    pkt.ipv4.src_ip = 0x0A000110;  // 10.0.1.16
    pkt.ipv4.ttl = 64;

    auto trace = pipeline.process(pkt);
    EXPECT_TRUE(trace.forwarded);
    EXPECT_EQ(trace.egress_port, 3);
    EXPECT_TRUE(trace.lpm_hit);
    EXPECT_EQ(trace.lpm_nexthop_id, 1);
}

TEST(Pipeline, DropOnIngressPortDown) {
    Pipeline pipeline;
    // Port 0 is down (default)

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.has_ipv4 = true;
    pkt.ipv4.dst_ip = 0x0A000214;
    pkt.ipv4.ttl = 64;

    auto trace = pipeline.process(pkt);
    EXPECT_FALSE(trace.forwarded);
}

TEST(Pipeline, AclDenyDropsPacket) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);
    pipeline.ports().set_admin_state(3, true);

    MacAddr nh_mac = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0x02};
    pipeline.nexthops().add(1, 3, nh_mac);
    pipeline.lpm().add(0x0A000200, 24, 1);

    // ACL: deny all traffic from 10.0.1.0/24
    AclRule rule{};
    rule.id = 1;
    rule.priority = 10;
    rule.src_ip = 0x0A000100;
    rule.src_ip_mask = 0xFFFFFF00;
    rule.action = AclAction::DENY;
    pipeline.acl().add(rule);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.has_ipv4 = true;
    pkt.ipv4.src_ip = 0x0A000110;  // 10.0.1.16 — matches deny rule
    pkt.ipv4.dst_ip = 0x0A000214;
    pkt.ipv4.ttl = 64;

    auto trace = pipeline.process(pkt);
    EXPECT_FALSE(trace.forwarded);
    EXPECT_EQ(trace.acl_action, AclAction::DENY);
}

}  // namespace asic::test
