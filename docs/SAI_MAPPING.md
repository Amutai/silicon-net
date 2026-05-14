# SAI → SDK → ASIC Mapping

This document traces how each SAI object type maps through the SDK to virtual ASIC tables.

## Object Type Mapping

| SAI Object | SAI API | SDK Call | ASIC Table | Key |
|---|---|---|---|---|
| Switch | `create_switch` | `sdk::initialize()` | — (global init) | — |
| Port | `set_port_attribute` | `sdk::port_set_admin_state()` | Port table | port_id |
| FDB Entry | `create_fdb_entry` | `sdk::fdb_add_entry()` | FDB table | {MAC, VLAN} |
| Route Entry | `create_route_entry` | `sdk::route_add()` | LPM table | {VR, prefix/len} |
| Next Hop | `create_next_hop` | `sdk::nexthop_create()` | Next-hop table | OID |
| ACL Table | `create_acl_table` | — (metadata only) | — | OID |
| ACL Entry | `create_acl_entry` | `sdk::acl_add_rule()` | ACL table | OID |

## SAI Attribute Pattern

All SAI create/set operations use attribute lists:

```c
sai_attribute_t attrs[3];
attrs[0].id = SAI_NEXT_HOP_ATTR_TYPE;
attrs[0].value.s32 = SAI_NEXT_HOP_TYPE_IP;
attrs[1].id = SAI_NEXT_HOP_ATTR_IP;
attrs[1].value.ipaddr = {.addr_family = SAI_IP_ADDR_FAMILY_IPV4, .addr.ip4 = 0xC0A80101};
attrs[2].id = SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID;
attrs[2].value.oid = rif_oid;

sai_next_hop_api->create_next_hop(&nh_oid, switch_oid, 3, attrs);
```

Our SAI implementation parses these attribute lists and translates to SDK structs:

```cpp
// Inside sai_create_next_hop():
silicon_sdk::nexthop_entry_t entry = {};
for (int i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
        case SAI_NEXT_HOP_ATTR_IP:
            entry.ip = attr_list[i].value.ipaddr.addr.ip4;
            break;
        case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
            entry.egress_port = oid_to_port(attr_list[i].value.oid);
            break;
    }
}
silicon_sdk::nexthop_id_t id;
return sdk_to_sai_status(silicon_sdk::nexthop_create(&entry, &id));
```

## OID vs Entry-Based Objects

**OID-based** (system assigns an ID):
- Next Hop, ACL Table, ACL Entry, Port (pre-created)
- Created with `create_*(&oid, switch_id, attr_count, attrs)`
- Referenced by other objects via the OID

**Entry-based** (caller provides the key):
- Route Entry (keyed by VR + prefix)
- FDB Entry (keyed by bridge + MAC)
- Neighbor Entry (keyed by RIF + IP)
- Created with `create_*_entry(&entry_key, attr_count, attrs)`

## Reference Graph

```
Route Entry ──references──→ Next Hop ──references──→ Port
                                                        ↑
FDB Entry ──────────────────────────────────────────────┘
                                                        ↑
ACL Entry (redirect action) ────────────────────────────┘
```

Deletion order must respect references. SDK enforces this via reference counting.

## Status Code Mapping

| SDK Status | SAI Status |
|---|---|
| `SDK_STATUS_OK` | `SAI_STATUS_SUCCESS` |
| `SDK_ERR_TABLE_FULL` | `SAI_STATUS_TABLE_FULL` |
| `SDK_ERR_NOT_FOUND` | `SAI_STATUS_ITEM_NOT_FOUND` |
| `SDK_ERR_EXISTS` | `SAI_STATUS_ITEM_ALREADY_EXISTS` |
| `SDK_ERR_IN_USE` | `SAI_STATUS_OBJECT_IN_USE` |
| `SDK_ERR_INVALID_PARAM` | `SAI_STATUS_INVALID_PARAMETER` |
