"""
silicon-net: orchagent-lite
Reads APP_DB, resolves dependencies, writes ASIC_DB.
"""

import redis
from nos.schema import (
    APP_DB, ASIC_DB,
    port_table_key, route_table_key, fdb_table_key,
    asic_nexthop_key, asic_route_key, asic_fdb_key, asic_port_key,
)


class OrchAgent:
    """Orchestrates APP_DB intent into ASIC_DB SAI objects."""

    def __init__(self, redis_host: str = "localhost", redis_port: int = 6379):
        self.app_db = redis.Redis(host=redis_host, port=redis_port, db=APP_DB, decode_responses=True)
        self.asic_db = redis.Redis(host=redis_host, port=redis_port, db=ASIC_DB, decode_responses=True)
        self._next_oid = 1

    def _allocate_oid(self) -> str:
        oid = f"oid:0x{self._next_oid:x}"
        self._next_oid += 1
        return oid

    def process_port(self, port_id: int):
        """Read PORT_TABLE entry and write to ASIC_DB."""
        key = port_table_key(port_id)
        data = self.app_db.hgetall(key)
        if not data:
            return

        admin_status = data.get("admin_status", "down")
        asic_key = asic_port_key(port_id)
        self.asic_db.hset(asic_key, mapping={
            "SAI_PORT_ATTR_ADMIN_STATE": "true" if admin_status == "up" else "false",
        })

    def process_route(self, prefix: str):
        """Read ROUTE_TABLE entry, create nexthop if needed, write route to ASIC_DB."""
        key = route_table_key(prefix)
        data = self.app_db.hgetall(key)
        if not data:
            return

        nexthop_ip = data.get("nexthop", "")
        ifname = data.get("ifname", "")
        port_id = int(ifname.replace("port", "")) if ifname.startswith("port") else 0

        # Create nexthop in ASIC_DB
        nh_oid = self._allocate_oid()
        nh_key = asic_nexthop_key(nh_oid)
        self.asic_db.hset(nh_key, mapping={
            "SAI_NEXT_HOP_ATTR_IP": nexthop_ip,
            "SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID": str(port_id),
        })

        # Create route in ASIC_DB
        route_key = asic_route_key(prefix)
        self.asic_db.hset(route_key, mapping={
            "SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID": nh_oid,
        })

    def process_fdb(self, vlan_id: int, mac: str):
        """Read FDB_TABLE entry and write to ASIC_DB."""
        key = fdb_table_key(vlan_id, mac)
        data = self.app_db.hgetall(key)
        if not data:
            return

        port_name = data.get("port", "")
        port_id = int(port_name.replace("port", "")) if port_name.startswith("port") else 0

        fdb_key = asic_fdb_key(mac, str(vlan_id))
        self.asic_db.hset(fdb_key, mapping={
            "SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID": str(port_id),
        })

    def sync_all(self):
        """Process all current APP_DB entries."""
        for key in self.app_db.scan_iter("PORT_TABLE:*"):
            port_name = key.split(":")[1]
            port_id = int(port_name.replace("port", ""))
            self.process_port(port_id)

        for key in self.app_db.scan_iter("ROUTE_TABLE:*"):
            prefix = key.split(":", 1)[1]
            self.process_route(prefix)

        for key in self.app_db.scan_iter("FDB_TABLE:*"):
            parts = key.split(":")
            vlan_id = int(parts[1].replace("Vlan", ""))
            mac = parts[2]
            self.process_fdb(vlan_id, mac)


if __name__ == "__main__":
    orch = OrchAgent()
    orch.sync_all()
