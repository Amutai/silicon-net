# silicon-net

A ground-up implementation of a simulated network ASIC with a SAI-compatible abstraction layer and a minimal SONiC-like Network Operating System. Built to deeply understand the SDK ↔ SAI ↔ NOS boundary that powers modern open networking platforms.

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│  Layer 4: Mini-NOS (Python)                                  │
│  ┌────────────┐   ┌─────────────┐  ┌───────────────────────┐ │
│  │    CLI     │   │  orchagent  │  │   routing daemon      │ │
│  └─────┬──────┘   └──────┬──────┘  └───────────┬───────────┘ │
│        └────────────┬────┘                     │             │
│                     ▼                          │             │
│              ┌─────────────┐                   │             │
│              │  Redis DB   │◄──────────────────┘             │
│              │ (APP/ASIC)  │                                 │
│              └──────┬──────┘                                 │
│                     ▼                                        │
│              ┌─────────────┐                                 │
│              │ syncd-lite  │                                 │
│              └──────┬──────┘                                 │
└─────────────────────┼────────────────────────────────────────┘
                      │ SAI API (C function calls)
┌─────────────────────┼────────────────────────────────────────┐
│  Layer 3: SAI       │ (C++)                                  │
│              ┌──────▼──────┐                                 │
│              │  SAI Impl   │  Implements sai*.h interfaces   │
│              └──────┬──────┘                                 │
└─────────────────────┼────────────────────────────────────────┘
                      │ SDK API calls
┌─────────────────────┼────────────────────────────────────────┐
│  Layer 2: SDK       │ (C++)                                  │
│              ┌──────▼──────┐                                 │
│              │ Silicon SDK │ Resource mgmt, table programming│
│              └──────┬──────┘                                 │
└─────────────────────┼────────────────────────────────────────┘
                      │ Direct table manipulation
┌─────────────────────┼────────────────────────────────────────┐
│  Layer 1: Virtual   │ ASIC (C++)                             │
│              ┌──────▼──────┐                                 │
│              │  Pipeline   │  Parser → L2 → L3 → ACL → Egress│
│              │  FDB Table  │  MAC address forwarding         │
│              │  LPM Table  │  Longest-prefix-match routing   │
│              │  ACL Table  │  Access control lists           │
│              │  Port Model │  Ingress/egress ports           │
│              └─────────────┘                                 │
└──────────────────────────────────────────────────────────────┘
```

## Goals

1. **Learn by building** — Understand how packets flow from NOS intent to ASIC forwarding
2. **SAI fluency** — Implement real SAI object models (routes, FDB, next-hops, ACLs)
3. **SDK design** — Experience the constraints of programming network silicon
4. **SONiC internals** — Replicate the Redis → syncd → SAI → SDK data path
5. **Demonstrate competence** — Concrete artifact for the Cisco Silicon One SDK role

## Build

```bash
# Prerequisites: CMake 3.16+, g++ with C++17, Python 3.10+, Redis
mkdir build && cd build
cmake ..
make -j$(nproc)
ctest --output-on-failure

# NOS layer
cd nos/
pip install -r requirements.txt
python -m pytest tests/
```

## Project Status

| Layer | Status |
|-------|--------|
| Virtual ASIC | ✅ Complete (48 tests) |
| SDK | ✅ Complete (19 tests) |
| SAI | ✅ Complete (7 tests) |
| Mini-NOS | ✅ Complete (6 tests) |
| Packet tools | 🔲 Not started |

## Build Order

See [SPEC.md](SPEC.md) for detailed milestones and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for design decisions.

## License

MIT
