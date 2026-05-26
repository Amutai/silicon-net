"""
silicon-net: orchagent-lite
Minimal SONiC orchagent equivalent — reads APP_DB, resolves dependencies, writes ASIC_DB.

In real SONiC:
  - orchagent has per-object-type "Orch" classes (RouteOrch, FdbOrch, etc.)
  - Each Orch subscribes to its APP_DB table
  - Resolves cross-object dependencies (route needs next-hop first)
  - Writes fully resolved SAI objects to ASIC_DB for syncd to consume

Our simplified version demonstrates the same orchestration logic.
"""

# TODO: Implement in Milestone 4

import redis


APP_DB = 0   # Redis DB index for application intent
ASIC_DB = 1  # Redis DB index for SAI objects


class OrchAgent:
    """Orchestrates APP_DB intent into ASIC_DB SAI objects."""

    def __init__(self, redis_host: str = "localhost", redis_port: int = 6379):
        self.app_db = redis.Redis(host=redis_host, port=redis_port, db=APP_DB)
        self.asic_db = redis.Redis(host=redis_host, port=redis_port, db=ASIC_DB)
        self.pubsub = self.app_db.pubsub()
        self._next_oid = 1
        self.running = False

    def start(self):
        """Subscribe to APP_DB tables and process intent."""
        self.running = True
        self.pubsub.psubscribe("__keyspace@{}__:*_TABLE:*".format(APP_DB))
        # TODO: event loop

    def stop(self):
        self.running = False
        self.pubsub.close()

    def _allocate_oid(self) -> str:
        oid = f"oid:0x{self._next_oid:x}"
        self._next_oid += 1
        return oid

    def process_route(self, key: str, data: dict):
        """
        APP_DB key: ROUTE_TABLE:10.0.0.0/24
        APP_DB value: {"nexthop": "192.168.1.1", "ifname": "port3"}

        → Resolve next-hop OID (create if needed)
        → Write to ASIC_DB as SAI_OBJECT_TYPE_ROUTE_ENTRY
        """
        # TODO: implement
        pass

    def process_fdb(self, key: str, data: dict):
        """
        APP_DB key: FDB_TABLE:Vlan1:00:11:22:33:44:55
        APP_DB value: {"port": "port2", "type": "static"}

        → Write to ASIC_DB as SAI_OBJECT_TYPE_FDB_ENTRY
        """
        # TODO: implement
        pass


if __name__ == "__main__":
    orch = OrchAgent()
    orch.start()
