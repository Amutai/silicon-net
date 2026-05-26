"""
silicon-net: syncd-lite
Minimal SONiC syncd equivalent — subscribes to ASIC_DB and calls SAI APIs.

In real SONiC:
  - syncd subscribes to ASIC_DB via Redis pub/sub
  - Deserializes SAI object attributes from Redis hash entries
  - Calls the appropriate SAI API (create/remove/set)
  - Records operations for warm boot replay

Our simplified version demonstrates the same data flow.
"""

# TODO: Implement in Milestone 4

import redis


ASIC_DB = 1  # Redis DB index (SONiC convention)


class SyncdLite:
    """Bridge between Redis ASIC_DB and SAI shared library."""

    def __init__(self, sai_lib_path: str, redis_host: str = "localhost", redis_port: int = 6379):
        self.redis = redis.Redis(host=redis_host, port=redis_port, db=ASIC_DB)
        self.pubsub = self.redis.pubsub()
        # TODO: Load SAI .so via ctypes
        # self.sai = ctypes.CDLL(sai_lib_path)
        self.running = False

    def start(self):
        """Subscribe to ASIC_DB keyspace notifications and process events."""
        self.running = True
        self.pubsub.psubscribe("__keyspace@{}__:ASIC_STATE:*".format(ASIC_DB))
        # TODO: event loop processing ASIC_DB changes → SAI calls

    def stop(self):
        self.running = False
        self.pubsub.close()

    def _process_event(self, message):
        """Parse ASIC_DB key and call appropriate SAI function."""
        # Key format: ASIC_STATE:SAI_OBJECT_TYPE_XXX:oid:0xNNN
        # or:         ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:{json_key}
        # TODO: implement dispatch to SAI create/remove/set
        pass


if __name__ == "__main__":
    syncd = SyncdLite(sai_lib_path="./build/lib/libsai.so")
    syncd.start()
