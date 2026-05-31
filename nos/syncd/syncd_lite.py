"""
silicon-net: syncd-lite
Subscribes to ASIC_DB and translates entries to SAI API calls.
"""

import redis
from nos.schema import ASIC_DB


class SyncdLite:
    """Bridge between Redis ASIC_DB and SAI shared library."""

    def __init__(self, redis_host: str = "localhost", redis_port: int = 6379):
        self.db = redis.Redis(host=redis_host, port=redis_port, db=ASIC_DB, decode_responses=True)

    def process_port(self, key: str, data: dict):
        """Translate port ASIC_DB entry to SAI call."""
        admin_state = data.get("SAI_PORT_ATTR_ADMIN_STATE", "false")
        # In production: call sai_set_port_attribute via ctypes
        return {"action": "set_port", "admin_state": admin_state == "true"}

    def process_nexthop(self, key: str, data: dict):
        """Translate nexthop ASIC_DB entry to SAI call."""
        ip = data.get("SAI_NEXT_HOP_ATTR_IP", "")
        rif = data.get("SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID", "0")
        # In production: call sai_create_next_hop via ctypes
        return {"action": "create_nexthop", "ip": ip, "port": int(rif)}

    def process_route(self, key: str, data: dict):
        """Translate route ASIC_DB entry to SAI call."""
        nh_oid = data.get("SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID", "")
        # In production: call sai_create_route_entry via ctypes
        return {"action": "create_route", "nexthop_oid": nh_oid}

    def process_fdb(self, key: str, data: dict):
        """Translate FDB ASIC_DB entry to SAI call."""
        port_id = data.get("SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID", "0")
        # In production: call sai_create_fdb_entry via ctypes
        return {"action": "create_fdb", "port": int(port_id)}

    def process_entry(self, key: str):
        """Dispatch ASIC_DB key to appropriate handler."""
        data = self.db.hgetall(key)
        if not data:
            return None

        if "SAI_OBJECT_TYPE_PORT" in key:
            return self.process_port(key, data)
        elif "SAI_OBJECT_TYPE_NEXT_HOP" in key:
            return self.process_nexthop(key, data)
        elif "SAI_OBJECT_TYPE_ROUTE_ENTRY" in key:
            return self.process_route(key, data)
        elif "SAI_OBJECT_TYPE_FDB_ENTRY" in key:
            return self.process_fdb(key, data)
        return None

    def sync_all(self):
        """Process all current ASIC_DB entries."""
        results = []
        for key in self.db.scan_iter("ASIC_STATE:*"):
            result = self.process_entry(key)
            if result:
                results.append(result)
        return results


if __name__ == "__main__":
    syncd = SyncdLite()
    syncd.sync_all()
