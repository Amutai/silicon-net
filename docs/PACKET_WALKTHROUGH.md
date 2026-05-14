# Packet Walkthrough

Step-by-step trace of a packet through the virtual ASIC pipeline.

## Scenario

**Setup:**
- Port 0: connected to host A (MAC `00:aa:bb:cc:dd:01`, IP `10.0.1.10`)
- Port 3: connected to host B (MAC `00:aa:bb:cc:dd:02`, IP `10.0.2.20`)
- FDB: `00:aa:bb:cc:dd:02` on port 3
- Route: `10.0.2.0/24` → next-hop ID 1
- Next-hop 1: egress port 3, rewrite dst-MAC to `00:aa:bb:cc:dd:02`
- ACL: permit all (default)

**Packet:** Host A sends to Host B

```
Ethernet: dst=00:aa:bb:cc:dd:02  src=00:aa:bb:cc:dd:01  type=0x0800
IPv4:     src=10.0.1.10  dst=10.0.2.20  ttl=64  proto=TCP
```

## Pipeline Stages

### Stage 1: Ingress Port

```
Input:  Raw packet arrives on port 0
Action: Record ingress port metadata, increment port rx counter
Output: {packet, metadata: {ingress_port: 0}}
```

### Stage 2: Parser

```
Input:  Raw bytes
Action: Extract Ethernet header (dst_mac, src_mac, ethertype)
        If ethertype == 0x0800: extract IPv4 header (src_ip, dst_ip, ttl, proto)
Output: Parsed packet fields available for lookup stages
        {dst_mac: 00:aa:bb:cc:dd:02, src_mac: 00:aa:bb:cc:dd:01,
         dst_ip: 10.0.2.20, src_ip: 10.0.1.10, ttl: 64}
```

### Stage 3: FDB Lookup (L2)

```
Input:  dst_mac=00:aa:bb:cc:dd:02, vlan=1
Action: Lookup in FDB table → HIT (port 3)
        Also learn src_mac on ingress port (00:aa:bb:cc:dd:01 → port 0)
Output: {fdb_hit: true, fdb_port: 3}
Note:   If this were a pure L2 packet (no IP), forwarding decision is made here.
        Since it's IP, we continue to L3.
```

### Stage 4: LPM Lookup (L3)

```
Input:  dst_ip=10.0.2.20
Action: Longest prefix match in LPM table
        10.0.2.20 matches 10.0.2.0/24 → next-hop ID 1
Output: {lpm_hit: true, nexthop_id: 1}
Note:   L3 result overrides L2 for routed packets.
```

### Stage 5: ACL

```
Input:  All parsed fields + metadata (ingress port, matched route, etc.)
Action: Walk ACL rules in priority order
        Rule 0 (default): match any → permit
Output: {acl_action: PERMIT}
Note:   If DENY → packet dropped, increment ACL drop counter.
        If REDIRECT → override egress port.
```

### Stage 6: Next-Hop Resolution

```
Input:  nexthop_id=1
Action: Lookup next-hop table → {egress_port: 3, dst_mac_rewrite: 00:aa:bb:cc:dd:02}
        Decrement TTL: 64 → 63
        Rewrite src_mac to switch MAC (if routing)
        Rewrite dst_mac to next-hop MAC
Output: {egress_port: 3, modified_packet: {..., ttl: 63}}
```

### Stage 7: Egress Port

```
Input:  Final packet + egress port 3
Action: Increment port 3 tx counter
        Transmit packet
Output: Packet sent out port 3
```

## Final Packet on Wire (Port 3)

```
Ethernet: dst=00:aa:bb:cc:dd:02  src=<switch_mac>  type=0x0800
IPv4:     src=10.0.1.10  dst=10.0.2.20  ttl=63  proto=TCP
```

## Drop Scenarios

| Scenario | Stage | Result |
|---|---|---|
| Unknown dst_mac, no route | LPM | Miss → drop (or punt to CPU) |
| ACL deny rule matches | ACL | Drop, increment counter |
| TTL = 1 | Next-hop | Drop (TTL expired), send ICMP |
| Egress port down | Egress | Drop, increment port drop counter |
| FDB miss + no route | FDB/LPM | Flood to all ports in VLAN (L2) or drop (L3) |

## How to Trace in silicon-net

```bash
# Using the packet-trace tool
./tools/packet-trace --ingress 0 \
    --dst-mac 00:aa:bb:cc:dd:02 \
    --src-mac 00:aa:bb:cc:dd:01 \
    --src-ip 10.0.1.10 \
    --dst-ip 10.0.2.20

# Output:
# [INGRESS]  port=0 rx_count=1
# [PARSER]   eth={dst=00:aa:bb:cc:dd:02 src=00:aa:bb:cc:dd:01 type=IPv4}
#            ipv4={src=10.0.1.10 dst=10.0.2.20 ttl=64}
# [FDB]      lookup mac=00:aa:bb:cc:dd:02 vlan=1 → HIT port=3
# [LPM]      lookup ip=10.0.2.20 → HIT 10.0.2.0/24 nhop=1
# [ACL]      rule=0 match=any action=PERMIT
# [NEXTHOP]  id=1 → port=3 rewrite_mac=00:aa:bb:cc:dd:02 ttl=63
# [EGRESS]   port=3 tx_count=1
# RESULT:    FORWARDED port=0 → port=3
```
