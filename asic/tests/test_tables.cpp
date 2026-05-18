// silicon-net: Virtual ASIC Table Tests
// TODO: Implement in Milestone 1

#include <gtest/gtest.h>
#include "tables.h"

namespace asic::test {

// --- FDB Tests ---

TEST(FdbTable, AddAndLookup) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    ASSERT_TRUE(fdb.add(mac, 1, 3));
    auto result = fdb.lookup(mac, 1);
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.port_id, 3);
}

TEST(FdbTable, LookupMiss) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    auto result = fdb.lookup(mac, 1);
    EXPECT_FALSE(result.hit);
}

TEST(FdbTable, Remove) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    fdb.add(mac, 1, 3);
    ASSERT_TRUE(fdb.remove(mac, 1));
    auto result = fdb.lookup(mac, 1);
    EXPECT_FALSE(result.hit);
    EXPECT_EQ(fdb.count(), 0);
}

TEST(FdbTable, RemoveNonExistent) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    EXPECT_FALSE(fdb.remove(mac, 1));
}

TEST(FdbTable, DuplicateUpdatesPort) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    fdb.add(mac, 1, 3);
    fdb.add(mac, 1, 5);
    EXPECT_EQ(fdb.count(), 1);
    auto result = fdb.lookup(mac, 1);
    EXPECT_EQ(result.port_id, 5);
}

TEST(FdbTable, VlanIsolation) {
    FdbTable fdb;
    MacAddr mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    fdb.add(mac, 1, 3);
    fdb.add(mac, 2, 5);
    EXPECT_EQ(fdb.count(), 2);
    EXPECT_EQ(fdb.lookup(mac, 1).port_id, 3);
    EXPECT_EQ(fdb.lookup(mac, 2).port_id, 5);
}

TEST(FdbTable, TableFull) {
    FdbTable fdb;
    for (size_t i = 0; i < FDB_TABLE_SIZE; i++) {
        MacAddr mac = {0, 0, uint8_t(i >> 24), uint8_t(i >> 16), uint8_t(i >> 8), uint8_t(i)};
        ASSERT_TRUE(fdb.add(mac, 1, 0));
    }
    EXPECT_TRUE(fdb.full());
    MacAddr extra = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    EXPECT_FALSE(fdb.add(extra, 1, 0));
}

// --- LPM Tests ---

TEST(LpmTable, ExactMatch) {
    LpmTable lpm;
    lpm.add(0x0A000000, 24, 1);  // 10.0.0.0/24 → nhop 1
    auto result = lpm.lookup(0x0A000005);  // 10.0.0.5
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.nexthop_id, 1);
}

TEST(LpmTable, LongestPrefixWins) {
    LpmTable lpm;
    lpm.add(0x0A000000, 16, 1);  // 10.0.0.0/16 → nhop 1
    lpm.add(0x0A000100, 24, 2);  // 10.0.1.0/24 → nhop 2
    auto result = lpm.lookup(0x0A000105);  // 10.0.1.5
    EXPECT_TRUE(result.hit);
    EXPECT_EQ(result.nexthop_id, 2);  // /24 is more specific
}

TEST(LpmTable, Miss) {
    LpmTable lpm;
    lpm.add(0x0A000000, 24, 1);  // 10.0.0.0/24
    auto result = lpm.lookup(0x0B000001);  // 11.0.0.1 — no match
    EXPECT_FALSE(result.hit);
}

// --- Next-Hop Tests ---

TEST(NexthopTable, CreateAndLookup) {
    NexthopTable nh;
    MacAddr mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    ASSERT_TRUE(nh.add(1, 3, mac));
    auto entry = nh.lookup(1);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->egress_port, 3);
}

TEST(NexthopTable, RefCountPreventsDelete) {
    NexthopTable nh;
    MacAddr mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    nh.add(1, 3, mac);
    nh.increment_ref(1);
    EXPECT_FALSE(nh.remove(1));  // Can't delete, ref_count > 0
    nh.decrement_ref(1);
    EXPECT_TRUE(nh.remove(1));   // Now OK
}

// --- ACL Tests ---

TEST(AclTable, DenyRule) {
    AclTable acl;
    AclRule rule{};
    rule.id = 1;
    rule.priority = 10;
    rule.src_ip = 0x0A000000;      // 10.0.0.0
    rule.src_ip_mask = 0xFF000000; // /8
    rule.action = AclAction::DENY;
    acl.add(rule);

    auto result = acl.evaluate(0x0A010203, 0xC0A80101);  // src=10.1.2.3
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.action, AclAction::DENY);
}

TEST(AclTable, NoMatchPermits) {
    AclTable acl;
    auto result = acl.evaluate(0x0A000001, 0xC0A80101);
    // No rules → default permit (no match)
    EXPECT_FALSE(result.matched);
}

}  // namespace asic::test
