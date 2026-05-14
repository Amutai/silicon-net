// silicon-net: SAI Integration Tests
// TODO: Implement in Milestone 3

#include <gtest/gtest.h>
#include "sai.h"

namespace sai::test {

class SaiTest : public ::testing::Test {
protected:
    sai_object_id_t switch_id_ = SAI_NULL_OBJECT_ID;

    void SetUp() override {
        ASSERT_EQ(sai_create_switch(&switch_id_, 0, nullptr), SAI_STATUS_SUCCESS);
    }
    void TearDown() override {
        sai_remove_switch(switch_id_);
    }
};

TEST_F(SaiTest, CreateSwitch) {
    EXPECT_NE(switch_id_, SAI_NULL_OBJECT_ID);
}

TEST_F(SaiTest, CreateNextHop) {
    sai_attribute_t attrs[2];
    attrs[0].id = SAI_NEXT_HOP_ATTR_TYPE;
    attrs[0].value.s32 = SAI_NEXT_HOP_TYPE_IP;
    attrs[1].id = SAI_NEXT_HOP_ATTR_IP;
    attrs[1].value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    attrs[1].value.ipaddr.addr.ip4 = 0xC0A80101;  // 192.168.1.1

    sai_object_id_t nh_oid;
    EXPECT_EQ(sai_create_next_hop(&nh_oid, switch_id_, 2, attrs), SAI_STATUS_SUCCESS);
    EXPECT_NE(nh_oid, SAI_NULL_OBJECT_ID);

    EXPECT_EQ(sai_remove_next_hop(nh_oid), SAI_STATUS_SUCCESS);
}

TEST_F(SaiTest, CreateRoute) {
    // First create a next-hop
    sai_attribute_t nh_attrs[2];
    nh_attrs[0].id = SAI_NEXT_HOP_ATTR_TYPE;
    nh_attrs[0].value.s32 = SAI_NEXT_HOP_TYPE_IP;
    nh_attrs[1].id = SAI_NEXT_HOP_ATTR_IP;
    nh_attrs[1].value.ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    nh_attrs[1].value.ipaddr.addr.ip4 = 0xC0A80101;

    sai_object_id_t nh_oid;
    ASSERT_EQ(sai_create_next_hop(&nh_oid, switch_id_, 2, nh_attrs), SAI_STATUS_SUCCESS);

    // Create route pointing to next-hop
    sai_route_entry_t route{};
    route.switch_id = switch_id_;
    route.vr_id = SAI_NULL_OBJECT_ID;  // Default VR
    route.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    route.destination.addr.ip4 = 0x0A000200;   // 10.0.2.0
    route.destination.mask.ip4 = 0xFFFFFF00;   // /24

    sai_attribute_t route_attrs[1];
    route_attrs[0].id = SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID;
    route_attrs[0].value.oid = nh_oid;

    EXPECT_EQ(sai_create_route_entry(&route, 1, route_attrs), SAI_STATUS_SUCCESS);
    EXPECT_EQ(sai_remove_route_entry(&route), SAI_STATUS_SUCCESS);
    EXPECT_EQ(sai_remove_next_hop(nh_oid), SAI_STATUS_SUCCESS);
}

}  // namespace sai::test
