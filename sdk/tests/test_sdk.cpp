// silicon-net: SDK Unit Tests
// TODO: Implement in Milestone 2

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

TEST_F(SdkTest, DoubleInitFails) {
    silicon_sdk::SdkConfig config{};
    EXPECT_EQ(silicon_sdk::initialize(&config), silicon_sdk::Status::ERR_ALREADY_INITIALIZED);
}

TEST_F(SdkTest, PortAdminState) {
    EXPECT_EQ(silicon_sdk::port_set_admin_state(0, true), silicon_sdk::Status::OK);
    bool state = false;
    EXPECT_EQ(silicon_sdk::port_get_admin_state(0, &state), silicon_sdk::Status::OK);
    EXPECT_TRUE(state);
}

TEST_F(SdkTest, InvalidPortFails) {
    EXPECT_EQ(silicon_sdk::port_set_admin_state(99, true), silicon_sdk::Status::ERR_INVALID_PARAM);
}

TEST_F(SdkTest, FdbAddAndRemove) {
    silicon_sdk::FdbEntry entry{};
    entry.mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    entry.vlan = 1;
    entry.port = 3;
    EXPECT_EQ(silicon_sdk::fdb_add_entry(&entry), silicon_sdk::Status::OK);
    EXPECT_EQ(silicon_sdk::fdb_remove_entry(entry.mac, entry.vlan), silicon_sdk::Status::OK);
}

TEST_F(SdkTest, NexthopRefCountPreventsDelete) {
    silicon_sdk::NexthopEntry nh{};
    nh.ip = 0xC0A80101;
    nh.egress_port = 3;
    nh.dst_mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    silicon_sdk::NexthopId id;
    EXPECT_EQ(silicon_sdk::nexthop_create(&nh, &id), silicon_sdk::Status::OK);

    // Add route referencing this next-hop
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

}  // namespace sdk::test
