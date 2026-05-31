"""
silicon-net: Redis Schema Definition
Mirrors SONiC's APP_DB / ASIC_DB conventions.
"""

# Redis DB indices (SONiC convention)
APP_DB = 0
ASIC_DB = 1

# --- APP_DB Keys ---
# Written by CLI, consumed by orchagent

PORT_TABLE = "PORT_TABLE:{port_name}"
# Fields: admin_status (up|down)

ROUTE_TABLE = "ROUTE_TABLE:{prefix}"
# Fields: nexthop (IP), ifname (port name)

FDB_TABLE = "FDB_TABLE:Vlan{vlan_id}:{mac}"
# Fields: port (port name), type (static|dynamic)

# --- ASIC_DB Keys ---
# Written by orchagent, consumed by syncd-lite

ASIC_STATE_PREFIX = "ASIC_STATE:"

# OID-based objects
ASIC_NEXTHOP = ASIC_STATE_PREFIX + "SAI_OBJECT_TYPE_NEXT_HOP:{oid}"
# Fields: SAI_NEXT_HOP_ATTR_IP, SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID

ASIC_ACL_ENTRY = ASIC_STATE_PREFIX + "SAI_OBJECT_TYPE_ACL_ENTRY:{oid}"

# Entry-based objects
ASIC_ROUTE = ASIC_STATE_PREFIX + "SAI_OBJECT_TYPE_ROUTE_ENTRY:{route_key}"
# Fields: SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID

ASIC_FDB = ASIC_STATE_PREFIX + "SAI_OBJECT_TYPE_FDB_ENTRY:{fdb_key}"
# Fields: SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID

ASIC_PORT = ASIC_STATE_PREFIX + "SAI_OBJECT_TYPE_PORT:{oid}"
# Fields: SAI_PORT_ATTR_ADMIN_STATE


def port_table_key(port_id: int) -> str:
    return f"PORT_TABLE:port{port_id}"


def route_table_key(prefix: str) -> str:
    return f"ROUTE_TABLE:{prefix}"


def fdb_table_key(vlan_id: int, mac: str) -> str:
    return f"FDB_TABLE:Vlan{vlan_id}:{mac}"


def asic_nexthop_key(oid: str) -> str:
    return f"ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:{oid}"


def asic_route_key(prefix: str, vr_id: str = "0") -> str:
    return f'ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:{{"dest":"{prefix}","vr":"{vr_id}"}}'


def asic_fdb_key(mac: str, bv_id: str = "1") -> str:
    return f'ASIC_STATE:SAI_OBJECT_TYPE_FDB_ENTRY:{{"mac":"{mac}","bv_id":"{bv_id}"}}'


def asic_port_key(port_id: int) -> str:
    return f"ASIC_STATE:SAI_OBJECT_TYPE_PORT:oid:0x{port_id:x}"
