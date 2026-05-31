"""
silicon-net: NOS Integration Tests
Tests the full path: CLI → APP_DB → orchagent → ASIC_DB → syncd-lite
"""

import pytest

try:
    import redis
    REDIS_AVAILABLE = True
except ImportError:
    REDIS_AVAILABLE = False


def redis_running():
    """Check if Redis is available."""
    if not REDIS_AVAILABLE:
        return False
    try:
        r = redis.Redis(host="localhost", port=6379)
        r.ping()
        return True
    except (redis.ConnectionError, ConnectionRefusedError):
        return False


@pytest.mark.skipif(not redis_running(), reason="Redis not available")
class TestIntegration:
    """Integration tests requiring a running Redis instance."""

    def setup_method(self):
        """Flush test DBs before each test."""
        self.app_db = redis.Redis(host="localhost", port=6379, db=0, decode_responses=True)
        self.asic_db = redis.Redis(host="localhost", port=6379, db=1, decode_responses=True)
        self.app_db.flushdb()
        self.asic_db.flushdb()

    def teardown_method(self):
        self.app_db.flushdb()
        self.asic_db.flushdb()

    def test_port_cli_to_asic_db(self):
        """CLI port command → APP_DB → orchagent → ASIC_DB."""
        from nos.orchagent.orchagent import OrchAgent

        # Simulate CLI writing to APP_DB
        self.app_db.hset("PORT_TABLE:port0", mapping={"admin_status": "up"})

        # orchagent processes
        orch = OrchAgent()
        orch.process_port(0)

        # Verify ASIC_DB
        key = "ASIC_STATE:SAI_OBJECT_TYPE_PORT:oid:0x0"
        data = self.asic_db.hgetall(key)
        assert data["SAI_PORT_ATTR_ADMIN_STATE"] == "true"

    def test_route_cli_to_asic_db(self):
        """CLI route command → APP_DB → orchagent → ASIC_DB (nexthop + route)."""
        from nos.orchagent.orchagent import OrchAgent

        # Simulate CLI writing to APP_DB
        self.app_db.hset("ROUTE_TABLE:10.0.2.0/24", mapping={
            "nexthop": "192.168.1.1",
            "ifname": "port3",
        })

        # orchagent processes
        orch = OrchAgent()
        orch.process_route("10.0.2.0/24")

        # Verify nexthop created in ASIC_DB
        nh_key = "ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:oid:0x1"
        nh_data = self.asic_db.hgetall(nh_key)
        assert nh_data["SAI_NEXT_HOP_ATTR_IP"] == "192.168.1.1"
        assert nh_data["SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID"] == "3"

        # Verify route created in ASIC_DB
        route_keys = [k for k in self.asic_db.scan_iter("ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:*")]
        assert len(route_keys) == 1
        route_data = self.asic_db.hgetall(route_keys[0])
        assert route_data["SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID"] == "oid:0x1"

    def test_fdb_cli_to_asic_db(self):
        """CLI fdb command → APP_DB → orchagent → ASIC_DB."""
        from nos.orchagent.orchagent import OrchAgent

        # Simulate CLI writing to APP_DB
        self.app_db.hset("FDB_TABLE:Vlan1:00:11:22:33:44:55", mapping={
            "port": "port2",
            "type": "static",
        })

        # orchagent processes
        orch = OrchAgent()
        orch.process_fdb(1, "00:11:22:33:44:55")

        # Verify FDB in ASIC_DB
        fdb_keys = [k for k in self.asic_db.scan_iter("ASIC_STATE:SAI_OBJECT_TYPE_FDB_ENTRY:*")]
        assert len(fdb_keys) == 1
        fdb_data = self.asic_db.hgetall(fdb_keys[0])
        assert fdb_data["SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID"] == "2"

    def test_syncd_processes_asic_db(self):
        """syncd-lite reads ASIC_DB and produces SAI call descriptors."""
        from nos.syncd.syncd_lite import SyncdLite

        # Write directly to ASIC_DB (as orchagent would)
        self.asic_db.hset("ASIC_STATE:SAI_OBJECT_TYPE_PORT:oid:0x0", mapping={
            "SAI_PORT_ATTR_ADMIN_STATE": "true",
        })
        self.asic_db.hset("ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:oid:0x1", mapping={
            "SAI_NEXT_HOP_ATTR_IP": "192.168.1.1",
            "SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID": "3",
        })

        syncd = SyncdLite()
        results = syncd.sync_all()

        actions = [r["action"] for r in results]
        assert "set_port" in actions
        assert "create_nexthop" in actions

    def test_full_path_sync(self):
        """Full path: APP_DB → orchagent → ASIC_DB → syncd-lite."""
        from nos.orchagent.orchagent import OrchAgent
        from nos.syncd.syncd_lite import SyncdLite

        # CLI writes to APP_DB
        self.app_db.hset("PORT_TABLE:port0", mapping={"admin_status": "up"})
        self.app_db.hset("ROUTE_TABLE:10.0.0.0/24", mapping={
            "nexthop": "192.168.1.1",
            "ifname": "port3",
        })

        # orchagent syncs APP_DB → ASIC_DB
        orch = OrchAgent()
        orch.sync_all()

        # syncd reads ASIC_DB
        syncd = SyncdLite()
        results = syncd.sync_all()

        actions = [r["action"] for r in results]
        assert "set_port" in actions
        assert "create_nexthop" in actions
        assert "create_route" in actions


def test_placeholder():
    """Always passes — ensures pytest collects this file without Redis."""
    assert True
