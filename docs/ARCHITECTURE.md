# silicon-net — Architecture & Design Decisions

## Design Philosophy

This project mirrors the real-world architecture of open networking stacks (SONiC on Silicon One) but strips away the complexity that obscures understanding. Every simplification is documented so you can articulate what the real system does differently.

---

## Layered Responsibility Model

Each layer has exactly one job and communicates only with its immediate neighbors.

| Layer | Responsibility | Knows About |
|-------|---------------|-------------|
| Virtual ASIC | Forward packets using programmed tables | Nothing above it |
| SDK | Program ASIC tables, manage resources | ASIC internals only |
| SAI | Translate standard API to vendor SDK | SDK API only |
| NOS | Network intent → SAI calls via Redis | SAI API only |

**Why this matters:** In the real world, a bug at the SAI layer means you need to understand both what SONiC expects (above) and what the SDK provides (below). This layering trains that mental model.

---

## Key Design Decisions

### 1. Virtual ASIC: Fixed-Function Pipeline

**Decision:** Sequential pipeline stages, not a programmable P4-style pipeline.

**Why:** Silicon One uses a fixed pipeline for most operations. Understanding fixed-function constraints (you can't reorder stages, table sizes are fixed at init) is more relevant than P4 flexibility for this role.

**Real-world difference:** Silicon One has a more complex pipeline with multiple lookup stages, traffic managers, and recirculation paths. Our 6-stage pipeline captures the essential flow.

```
Real Silicon One (simplified):
  Parser → FLP (L2) → FEC (L3) → ACL → TM → Egress

Our model:
  Parser → FDB → LPM → ACL → Next-hop → Egress
```

### 2. SDK: Object-Oriented Resource Management

**Decision:** SDK owns all object lifecycles with reference counting.

**Why:** This is the core complexity of real SDK work. You can't delete a next-hop that routes still reference. You can't add a 1025th FDB entry to a 1024-entry table. The SDK must enforce these invariants.

**Real-world difference:** Real SDKs also handle:
- Hardware register programming (we use in-memory tables)
- DMA and memory-mapped I/O (we use direct struct access)
- Interrupt handling (we use synchronous calls)
- Multi-threaded access with locking (we're single-threaded initially)

### 3. SAI: Subset of Real SAI Headers

**Decision:** Implement a subset of SAI (~6 object types) using simplified but structurally accurate headers.

**Why:** Full SAI has 50+ object types. We implement the core ones that cover 90% of interview discussions: switch, port, FDB, route, next-hop, ACL. The patterns (OID vs entry-based, attribute lists) are identical to full SAI.

**Real-world difference:** Real SAI also covers:
- LAG, STP, VLAN, mirror, tunnel, QoS, buffer, scheduler
- Bulk operations (create_fdb_entries plural)
- Warm boot (state serialization/deserialization)

### 4. NOS: Redis as the Integration Bus

**Decision:** Use Redis exactly as SONiC does — APP_DB for intent, ASIC_DB for hardware state.

**Why:** This is SONiC's defining architectural choice. Understanding the pub/sub flow through Redis databases is essential for anyone working with SONiC. It's also what makes SONiC debuggable — you can inspect any DB to see system state.

**Real-world difference:** Real SONiC has:
- 7+ Redis databases (APP, ASIC, CONFIG, STATE, COUNTERS, etc.)
- Multiple orchagent threads for different object types
- Warm restart coordination between syncd and orchagent
- SAI Redis (sairedis) as a recording/playback layer

---

## Data Flow: Route Addition (End-to-End)

```
User types: "route add 10.0.0.0/24 nexthop 192.168.1.1 port 3"

1. CLI parses command
2. CLI writes to APP_DB:
   ROUTE_TABLE:10.0.0.0/24 = {"nexthop": "192.168.1.1", "ifname": "port3"}

3. orchagent receives APP_DB notification
4. orchagent resolves next-hop → creates SAI next-hop if needed
5. orchagent writes to ASIC_DB:
   ASIC_STATE:SAI_OBJECT_TYPE_NEXT_HOP:oid:0x1 = {"SAI_NEXT_HOP_ATTR_IP": "192.168.1.1", ...}
   ASIC_STATE:SAI_OBJECT_TYPE_ROUTE_ENTRY:{...} = {"SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID": "oid:0x1"}

6. syncd receives ASIC_DB notification
7. syncd calls SAI:
   sai_route_api->create_route_entry(&entry, attr_count, attr_list)

8. SAI implementation calls SDK:
   silicon_sdk::nexthop_create(...)
   silicon_sdk::route_add(...)

9. SDK programs Virtual ASIC:
   lpm_table.insert(prefix=10.0.0.0/24, nexthop_id=1)
   nexthop_table.insert(id=1, port=3, dst_mac=...)

10. Next packet to 10.0.0.5 hitting the ASIC:
    Parser → FDB miss → LPM hit (10.0.0.0/24) → ACL pass → NH resolve (port 3) → Egress port 3
```

---

## What This Teaches You (Interview Talking Points)

| Topic | What You Can Say |
|-------|-----------------|
| SAI object model | "SAI uses two patterns: OID-based objects like next-hops that get a unique ID, and entry-based objects like routes that are keyed by their content" |
| Resource management | "The SDK tracks table occupancy and returns errors on exhaustion — you can't just malloc in hardware" |
| Reference counting | "Routes hold references to next-hops. The SDK refuses to delete a next-hop with active references" |
| SONiC data path | "Intent flows through Redis databases: CLI → APP_DB → orchagent → ASIC_DB → syncd → SAI → SDK → hardware" |
| Debugging | "I can inspect any Redis DB to see where state diverged, or trace a packet through pipeline stages" |
| Pipeline model | "Fixed-function pipelines process packets in deterministic stage order — you design your tables to match the pipeline, not the other way around" |

---

## Simplifications & Future Extensions

| Simplification | Real-World Equivalent | Future Extension |
|---|---|---|
| 8 ports | 256+ ports with SerDes | Configurable port count |
| Single thread | Multi-threaded with locks | Add mutex to SDK |
| In-memory tables | TCAM/SRAM hardware | Simulate access latency |
| No warm boot | State save/restore | Add SAI warm boot |
| No LAG/ECMP | Link aggregation, equal-cost multipath | Add ECMP next-hop groups |
| IPv4 only | IPv4 + IPv6 + MPLS | Add IPv6 LPM |
| No QoS | Traffic classes, scheduling | Add queue model |
