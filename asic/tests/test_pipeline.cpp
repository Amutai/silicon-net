// silicon-net: Virtual ASIC Pipeline Tests
// TODO: Implement in Milestone 1

#include <gtest/gtest.h>
#include "pipeline.h"

namespace asic::test {

TEST(Pipeline, ForwardL3Packet) {
    GTEST_SKIP() << "Requires LPM, nexthop, and egress stages (#7, #9, #10)";
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
    EXPECT_TRUE(pkt.dropped);
    EXPECT_STREQ(trace.drop_reason, "ingress port down");
}

TEST(Pipeline, IngressIncrementsRxCounter) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.ethertype = 0x0800;
    pkt.ipv4.dst_ip = 0x0A000214;
    pkt.ipv4.ttl = 64;

    pipeline.process(pkt);
    auto* port = pipeline.ports().get_port(0);
    EXPECT_EQ(port->counters.rx_packets, 1);
}

TEST(Pipeline, L2ForwardOnFdbHit) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);
    pipeline.ports().set_admin_state(2, true);

    MacAddr dst = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    pipeline.fdb().add(dst, 1, 2);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.dst_mac = dst;
    pkt.eth.ethertype = 0x0800;

    auto trace = pipeline.process(pkt);
    EXPECT_TRUE(trace.fdb_hit);
    EXPECT_EQ(trace.fdb_port, 2);
    EXPECT_EQ(pkt.egress_port, 2);
    EXPECT_FALSE(pkt.has_ipv4);  // L3 skipped
    EXPECT_FALSE(pkt.dropped);
}

TEST(Pipeline, DropNonIpv4OnFdbMiss) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    pkt.eth.ethertype = 0x0806;  // ARP, not IPv4

    auto trace = pipeline.process(pkt);
    EXPECT_FALSE(trace.fdb_hit);
    EXPECT_TRUE(pkt.dropped);
    EXPECT_STREQ(trace.drop_reason, "no FDB hit and not IPv4");
}

TEST(Pipeline, LpmHitSetsNexthop) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);
    pipeline.lpm().add(0x0A000200, 24, 42);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.ethertype = 0x0800;
    pkt.ipv4.dst_ip = 0x0A000205;
    pkt.ipv4.ttl = 64;

    auto trace = pipeline.process(pkt);
    EXPECT_TRUE(trace.lpm_hit);
    EXPECT_EQ(trace.lpm_nexthop_id, 42);
}

TEST(Pipeline, LpmMissDropsPacket) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.ethertype = 0x0800;
    pkt.ipv4.dst_ip = 0x0B000001;  // No route for 11.x.x.x
    pkt.ipv4.ttl = 64;

    auto trace = pipeline.process(pkt);
    EXPECT_FALSE(trace.lpm_hit);
    EXPECT_TRUE(pkt.dropped);
    EXPECT_STREQ(trace.drop_reason, "no route");
}

TEST(Pipeline, LpmSkippedOnFdbHit) {
    Pipeline pipeline;
    pipeline.ports().set_admin_state(0, true);
    pipeline.ports().set_admin_state(2, true);

    MacAddr dst = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    pipeline.fdb().add(dst, 1, 2);

    Packet pkt{};
    pkt.ingress_port = 0;
    pkt.eth.dst_mac = dst;
    pkt.eth.ethertype = 0x0800;

    auto trace = pipeline.process(pkt);
    EXPECT_TRUE(trace.fdb_hit);
    EXPECT_FALSE(trace.lpm_hit);  // LPM not consulted
}

TEST(Pipeline, AclDenyDropsPacket) {
    GTEST_SKIP() << "Requires LPM and ACL stages (#7, #8)";
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
