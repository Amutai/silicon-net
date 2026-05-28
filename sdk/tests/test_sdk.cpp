// silicon-net: SDK Unit Tests

#include <gtest/gtest.h>
#include "silicon_sdk.h"

namespace sdk::test {

class SdkTest : public ::testing::Test {
protected:
    void SetUp() override {
        silicon_sdk::SdkConfig config{};
        ASSERT_EQ(silicon_sdk::initialize(&config), silicon_sdk::Status::OK);
    }
    void TearDown() override {
        silicon_sdk::shutdown();
    }
};

// --- Lifecycle (#14) ---

TEST_F(SdkTest, DoubleInitFails) {
    silicon_sdk::SdkConfig config{};
    EXPECT_EQ(silicon_sdk::initialize(&config), silicon_sdk::Status::ERR_ALREADY_INITIALIZED);
}

TEST_F(SdkTest, ShutdownAndReinit) {
    EXPECT_EQ(silicon_sdk::shutdown(), silicon_sdk::Status::OK);
    silicon_sdk::SdkConfig config{};
    EXPECT_EQ(silicon_sdk::initialize(&config), silicon_sdk::Status::OK);
}

TEST_F(SdkTest, UseAfterShutdownFails) {
    silicon_sdk::shutdown();
    EXPECT_EQ(silicon_sdk::port_set_admin_state(0, true), silicon_sdk::Status::ERR_NOT_INITIALIZED);
}

// --- Port Operations (#17) ---

TEST_F(SdkTest, PortAdminState) {
    EXPECT_EQ(silicon_sdk::port_set_admin_state(0, true), silicon_sdk::Status::OK);
    bool state = false;
    EXPECT_EQ(silicon_sdk::port_get_admin_state(0, &state), silicon_sdk::Status::OK);
    EXPECT_TRUE(state);
}

TEST_F(SdkTest, InvalidPortFails) {
    EXPECT_EQ(silicon_sdk::port_set_admin_state(99, true), silicon_sdk::Status::ERR_INVALID_PARAM);
}

TEST_F(SdkTest, PortGetCounters) {
    silicon_sdk::PortCounters counters{};
    EXPECT_EQ(silicon_sdk::port_get_counters(0, &counters), silicon_sdk::Status::OK);
    EXPECT_EQ(counters.rx_packets, 0);
}

TEST_F(SdkTest, PortGetCountersInvalidPort) {
    silicon_sdk::PortCounters counters{};
    EXPECT_EQ(silicon_sdk::port_get_counters(99, &counters), silicon_sdk::Status::ERR_INVALID_PARAM);
}

TEST_F(SdkTest, PortGetCountersNullPtr) {
    EXPECT_EQ(silicon_sdk::port_get_counters(0, nullptr), silicon_sdk::Status::ERR_INVALID_PARAM);
}

// --- FDB Operations (#18) ---

TEST_F(SdkTest, FdbAddAndLookup) {
    silicon_sdk::FdbEntry entry{};
    entry.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    entry.vlan = 1;
    entry.port = 3;
    EXPECT_EQ(silicon_sdk::fdb_add_entry(&entry), silicon_sdk::Status::OK);

    silicon_sdk::FdbEntry out{};
    EXPECT_EQ(silicon_sdk::fdb_get_entry(entry.mac, entry.vlan, &out), silicon_sdk::Status::OK);
    EXPECT_EQ(out.port, 3);
}

TEST_F(SdkTest, FdbAddAndRemove) {
    silicon_sdk::FdbEntry entry{};
    entry.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    entry.vlan = 1;
    entry.port = 3;
    EXPECT_EQ(silicon_sdk::fdb_add_entry(&entry), silicon_sdk::Status::OK);
    EXPECT_EQ(silicon_sdk::fdb_remove_entry(entry.mac, entry.vlan), silicon_sdk::Status::OK);

    silicon_sdk::FdbEntry out{};
    EXPECT_EQ(silicon_sdk::fdb_get_entry(entry.mac, entry.vlan, &out), silicon_sdk::Status::ERR_NOT_FOUND);
}

TEST_F(SdkTest, FdbRemoveNonExistent) {
    silicon_sdk::MacAddr mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_EQ(silicon_sdk::fdb_remove_entry(mac, 1), silicon_sdk::Status::ERR_NOT_FOUND);
}

// --- NextHop Operations (#20) ---

TEST_F(SdkTest, NexthopCreateAndRemove) {
    silicon_sdk::NexthopEntry nh{};
    nh.egress_port = 3;
    nh.dst_mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    silicon_sdk::NexthopId id;
    EXPECT_EQ(silicon_sdk::nexthop_create(&nh, &id), silicon_sdk::Status::OK);
    EXPECT_EQ(silicon_sdk::nexthop_remove(id), silicon_sdk::Status::OK);
}

TEST_F(SdkTest, NexthopRemoveNonExistent) {
    EXPECT_EQ(silicon_sdk::nexthop_remove(999), silicon_sdk::Status::ERR_NOT_FOUND);
}

// --- Route Operations (#19) ---

TEST_F(SdkTest, RouteAddRequiresValidNexthop) {
    silicon_sdk::RouteEntry route{};
    route.prefix = 0x0A000200;
    route.prefix_len = 24;
    route.nexthop_id = 999;  // doesn't exist
    EXPECT_EQ(silicon_sdk::route_add(&route), silicon_sdk::Status::ERR_NOT_FOUND);
}

TEST_F(SdkTest, NexthopRefCountPreventsDelete) {
    silicon_sdk::NexthopEntry nh{};
    nh.egress_port = 3;
    nh.dst_mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    silicon_sdk::NexthopId id;
    EXPECT_EQ(silicon_sdk::nexthop_create(&nh, &id), silicon_sdk::Status::OK);

    silicon_sdk::RouteEntry route{};
    route.prefix = 0x0A000200;
    route.prefix_len = 24;
    route.nexthop_id = id;
    EXPECT_EQ(silicon_sdk::route_add(&route), silicon_sdk::Status::OK);

    // Can't delete next-hop while route references it
    EXPECT_EQ(silicon_sdk::nexthop_remove(id), silicon_sdk::Status::ERR_IN_USE);

    // Remove route first, then next-hop
    EXPECT_EQ(silicon_sdk::route_remove(route.prefix, route.prefix_len), silicon_sdk::Status::OK);
    EXPECT_EQ(silicon_sdk::nexthop_remove(id), silicon_sdk::Status::OK);
}

// --- ACL Operations (#21) ---

TEST_F(SdkTest, AclAddAndRemove) {
    silicon_sdk::AclRule rule{};
    rule.priority = 10;
    rule.src_ip = 0x0A000000;
    rule.src_ip_mask = 0xFF000000;
    rule.action = silicon_sdk::AclAction::DENY;

    silicon_sdk::AclRuleId id;
    EXPECT_EQ(silicon_sdk::acl_add_rule(&rule, &id), silicon_sdk::Status::OK);
    EXPECT_EQ(silicon_sdk::acl_remove_rule(id), silicon_sdk::Status::OK);
}

TEST_F(SdkTest, AclRemoveNonExistent) {
    EXPECT_EQ(silicon_sdk::acl_remove_rule(999), silicon_sdk::Status::ERR_NOT_FOUND);
}

// --- Packet Injection (#22) ---

TEST_F(SdkTest, InjectPacketL3Forward) {
    silicon_sdk::port_set_admin_state(0, true);
    silicon_sdk::port_set_admin_state(3, true);

    silicon_sdk::NexthopEntry nh{};
    nh.egress_port = 3;
    nh.dst_mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    silicon_sdk::NexthopId nhid;
    silicon_sdk::nexthop_create(&nh, &nhid);

    silicon_sdk::RouteEntry route{};
    route.prefix = 0x0A000200;
    route.prefix_len = 24;
    route.nexthop_id = nhid;
    silicon_sdk::route_add(&route);

    silicon_sdk::PacketDesc desc{};
    desc.ingress_port = 0;
    desc.dst_ip = 0x0A000205;
    desc.src_ip = 0xC0A80001;
    desc.ttl = 64;

    silicon_sdk::PacketResult result{};
    EXPECT_EQ(silicon_sdk::inject_packet(&desc, &result), silicon_sdk::Status::OK);
    EXPECT_TRUE(result.forwarded);
    EXPECT_EQ(result.egress_port, 3);
}

TEST_F(SdkTest, InjectPacketNoRoute) {
    silicon_sdk::port_set_admin_state(0, true);

    silicon_sdk::PacketDesc desc{};
    desc.ingress_port = 0;
    desc.dst_ip = 0x0B000001;
    desc.ttl = 64;

    silicon_sdk::PacketResult result{};
    EXPECT_EQ(silicon_sdk::inject_packet(&desc, &result), silicon_sdk::Status::OK);
    EXPECT_FALSE(result.forwarded);
}

}  // namespace sdk::test
