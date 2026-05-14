// silicon-net: Virtual ASIC Port Tests
// TODO: Implement in Milestone 1

#include <gtest/gtest.h>
#include "port.h"

namespace asic::test {

TEST(PortTable, InitialStateDown) {
    PortTable ports;
    EXPECT_FALSE(ports.is_up(0));
}

TEST(PortTable, SetAdminUp) {
    PortTable ports;
    ASSERT_TRUE(ports.set_admin_state(0, true));
    EXPECT_TRUE(ports.is_up(0));
}

TEST(PortTable, InvalidPort) {
    PortTable ports;
    EXPECT_FALSE(ports.set_admin_state(99, true));
    EXPECT_FALSE(ports.is_up(99));
}

TEST(PortTable, Counters) {
    PortTable ports;
    ports.increment_rx(0);
    ports.increment_rx(0);
    ports.increment_tx(0);
    auto* p = ports.get_port(0);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->counters.rx_packets, 2);
    EXPECT_EQ(p->counters.tx_packets, 1);
}

}  // namespace asic::test
