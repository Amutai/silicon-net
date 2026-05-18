# silicon-net — Project Specification

## Overview

A lite, educational implementation of the full network switch software stack: from a virtual ASIC forwarding engine, through a hardware SDK, a SAI abstraction layer, up to a SONiC-like NOS. The project demonstrates deep understanding of how open standard network operating systems interact with silicon.

## Target Stack

SDK development, SAI integration, and open NOS platforms (SONiC, FBOSS).

---

## Component Specifications

### 1. Virtual ASIC (`asic/`)

A software simulation of a network ASIC's forwarding engine.

**Data structures:**
- **Port table** — 8 virtual ports, each with admin state, speed, MTU, counters
- **FDB table** — 1024-entry MAC address table (VLAN + MAC → port mapping)
- **LPM table** — 512-entry longest-prefix-match routing table (prefix/len → next-hop)
- **ACL table** — 256-entry access control list (match fields → permit/deny/redirect)
- **Next-hop table** — 128 entries (next-hop ID → egress port + dst MAC rewrite)

**Packet pipeline (sequential stages):**
```
Ingress Port → Parser → FDB Lookup → L3 Lookup (LPM) → ACL → Next-hop Resolution → Egress Port
```

**Constraints (simulating real silicon):**
- Fixed table sizes (resource exhaustion returns errors)
- Pipeline is fixed-function (stages execute in order, no reordering)
- Counters increment atomically per-table-hit
- No dynamic memory allocation in the hot path

**Deliverables:**
- `pipeline.h/cpp` — Packet processing pipeline
- `tables.h/cpp` — Table data structures with CRUD operations
- `port.h/cpp` — Port model with state and counters
- `packet.h` — Packet representation (Ethernet + IPv4 headers)
- Unit tests for each table and pipeline stage

---

### 2. SDK (`sdk/`)

The programming interface to the virtual ASIC. Analogous to Cisco's Silicon One SDK.

**Responsibilities:**
- Translate high-level operations into table entries
- Manage object lifecycles (create/get/set/remove)
- Reference counting (e.g., next-hop can't be deleted while routes point to it)
- Resource tracking (return `SDK_ERR_TABLE_FULL` when capacity exceeded)
- Bulk operations for efficiency
- Object ID generation and mapping

**Public API (`silicon_sdk.h`):**
```cpp
namespace silicon_sdk {
    // Lifecycle
    sdk_status_t initialize(sdk_config_t* config);
    sdk_status_t shutdown();

    // Port operations
    sdk_status_t port_set_admin_state(port_id_t port, bool up);
    sdk_status_t port_get_counters(port_id_t port, port_counters_t* out);

    // FDB operations
    sdk_status_t fdb_add_entry(fdb_entry_t* entry);
    sdk_status_t fdb_remove_entry(mac_addr_t mac, vlan_id_t vlan);
    sdk_status_t fdb_get_entry(mac_addr_t mac, vlan_id_t vlan, fdb_entry_t* out);

    // Route operations
    sdk_status_t route_add(route_entry_t* entry);
    sdk_status_t route_remove(ip_prefix_t* prefix);

    // Next-hop operations
    sdk_status_t nexthop_create(nexthop_entry_t* entry, nexthop_id_t* out_id);
    sdk_status_t nexthop_remove(nexthop_id_t id);

    // ACL operations
    sdk_status_t acl_add_rule(acl_rule_t* rule, acl_rule_id_t* out_id);
    sdk_status_t acl_remove_rule(acl_rule_id_t id);

    // Packet injection (for testing)
    sdk_status_t inject_packet(port_id_t ingress, const uint8_t* pkt, size_t len);
}
```

**Deliverables:**
- `silicon_sdk.h` — Public API header
- `sdk_types.h` — Type definitions (entries, IDs, status codes)
- `sdk.cpp` — Implementation with resource management
- Unit tests for all operations including error paths

---

### 3. SAI Implementation (`sai/`)

Implements the [OCP SAI](https://github.com/opencomputeproject/SAI) interface against our SDK.

**SAI objects to implement:**
- `sai_switch` — Switch-level attributes and initialization
- `sai_port` — Port state, speed, counters
- `sai_fdb` — FDB entry create/remove/get
- `sai_route` — Route entry operations
- `sai_next_hop` — Next-hop create/remove
- `sai_acl` — ACL table and entry management
- `sai_virtual_router` — VRF (single default VR for now)

**Key SAI patterns to implement:**
- Object ID (OID) based references
- Attribute list pattern (`sai_attribute_t` arrays)
- Entry-based vs OID-based objects (routes are entry-based, next-hops are OID-based)
- Notification callbacks (FDB learn events, port state change)

**Deliverables:**
- SAI header stubs in `inc/` (simplified versions of real SAI headers)
- Implementation files mapping SAI calls → SDK calls
- SAI attribute validation
- Unit tests verifying SAI semantics

---

### 4. Mini-NOS (`nos/`)

A minimal SONiC-like NOS demonstrating the full control path.

**Components:**

**syncd-lite** (`nos/syncd/`)
- Subscribes to Redis ASIC_DB
- Translates DB entries into SAI API calls (via ctypes/cffi binding to SAI .so)
- Handles object creation ordering (next-hop before route)

**orchagent** (`nos/orchagent/`)
- Subscribes to Redis APP_DB
- Applies orchestration logic (resolve dependencies, ordering)
- Writes to ASIC_DB

**CLI** (`nos/cli/`)
- Simple command interface:
  ```
  > port 0 up
  > fdb add 00:11:22:33:44:55 vlan 1 port 2
  > route add 10.0.0.0/24 nexthop 192.168.1.1 port 3
  > acl add src-ip 10.0.0.0/8 action deny
  > show fdb
  > show routes
  > show counters
  > inject-packet port 0 dst-mac 00:11:22:33:44:55 src-ip 10.0.1.1 dst-ip 10.0.0.5
  ```

**Redis schema (mirrors SONiC):**
```
APP_DB:
  ROUTE_TABLE:10.0.0.0/24 → {"nexthop": "192.168.1.1", "ifname": "port3"}
  FDB_TABLE:Vlan1:00:11:22:33:44:55 → {"port": "port2", "type": "static"}

ASIC_DB:
  ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:{...} → {"SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID": "oid:0x1"}
  ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:oid:0x1 → {"SAI_NEXT_HOP_ATTR_IP": "192.168.1.1"}
```

**Deliverables:**
- syncd-lite with SAI bindings
- orchagent with APP_DB → ASIC_DB translation
- CLI for interactive use and demos
- Integration tests for full path

---

### 5. Tools (`tools/`)

- `pkt-inject` — Craft and inject packets into the virtual ASIC
- `table-dump` — Dump all ASIC table contents
- `packet-trace` — Show pipeline stage-by-stage decisions for a packet

---

## Build Order (Milestones)

### Milestone 1: Virtual ASIC
- [ ] Packet struct (Ethernet + IPv4)
- [ ] Port model with state and counters
- [ ] FDB table with lookup
- [ ] LPM table with longest-prefix-match
- [ ] ACL table with match/action
- [ ] Pipeline: wire stages together
- [ ] Unit tests for all components
- [ ] Packet walkthrough test (inject → forward)

### Milestone 2: SDK
- [ ] Type definitions and error codes
- [ ] SDK initialization and shutdown
- [ ] Port operations
- [ ] FDB operations with resource limits
- [ ] Route operations with next-hop references
- [ ] Next-hop lifecycle with reference counting
- [ ] ACL operations
- [ ] Packet injection through SDK
- [ ] Unit tests including error/edge cases

### Milestone 3: SAI Layer
- [ ] SAI type definitions (simplified sai.h, saitypes.h)
- [ ] sai_switch implementation
- [ ] sai_port implementation
- [ ] sai_fdb implementation
- [ ] sai_route + sai_next_hop implementation
- [ ] sai_acl implementation
- [ ] Notification callbacks
- [ ] SAI conformance tests

### Milestone 4: Mini-NOS
- [ ] Redis schema definition
- [ ] syncd-lite (ASIC_DB subscriber → SAI calls)
- [ ] orchagent (APP_DB → ASIC_DB)
- [ ] CLI (user commands → APP_DB)
- [ ] Integration test: CLI → Redis → syncd → SAI → SDK → ASIC → packet forwarded

### Milestone 5: Tools & Polish
- [ ] Packet injection tool
- [ ] Table dump tool
- [ ] Packet trace tool
- [ ] End-to-end demo script
- [ ] Documentation (PACKET_WALKTHROUGH.md, SAI_MAPPING.md)

---

## How to Use This Spec

Tell Amazon Q:
> "Read SPEC.md and start building Milestone 1 of silicon-net."

Each milestone is self-contained and testable before moving to the next.

---

## References

- [OCP SAI GitHub](https://github.com/opencomputeproject/SAI)
- [SONiC Architecture](https://github.com/sonic-net/SONiC/wiki/Architecture)
- [SONiC syncd](https://github.com/sonic-net/sonic-sairedis)
- [SAI object model](https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal.md)
