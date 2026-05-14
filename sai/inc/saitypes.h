#pragma once
// silicon-net: Simplified SAI Type Definitions
// Based on OCP SAI (github.com/opencomputeproject/SAI)
// Subset for educational purposes.

#include <cstdint>
#include <cstddef>

// --- SAI Status ---
typedef int32_t sai_status_t;

#define SAI_STATUS_SUCCESS              0x00000000
#define SAI_STATUS_FAILURE              0x00000001
#define SAI_STATUS_NOT_SUPPORTED        0x00000002
#define SAI_STATUS_NO_MEMORY            0x00000003
#define SAI_STATUS_INSUFFICIENT_RESOURCES 0x00000004
#define SAI_STATUS_INVALID_PARAMETER    0x00000005
#define SAI_STATUS_ITEM_ALREADY_EXISTS  0x00000006
#define SAI_STATUS_ITEM_NOT_FOUND       0x00000007
#define SAI_STATUS_TABLE_FULL           0x00000008
#define SAI_STATUS_OBJECT_IN_USE        0x00000009

// --- SAI Object ID ---
typedef uint64_t sai_object_id_t;

#define SAI_NULL_OBJECT_ID 0

// --- SAI Object Types ---
typedef enum _sai_object_type_t {
    SAI_OBJECT_TYPE_NULL = 0,
    SAI_OBJECT_TYPE_SWITCH,
    SAI_OBJECT_TYPE_PORT,
    SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
    SAI_OBJECT_TYPE_NEXT_HOP,
    SAI_OBJECT_TYPE_ACL_TABLE,
    SAI_OBJECT_TYPE_ACL_ENTRY,
    SAI_OBJECT_TYPE_FDB_ENTRY,
    SAI_OBJECT_TYPE_ROUTE_ENTRY,
    SAI_OBJECT_TYPE_MAX
} sai_object_type_t;

// --- SAI IP Address ---
typedef enum _sai_ip_addr_family_t {
    SAI_IP_ADDR_FAMILY_IPV4 = 0,
    SAI_IP_ADDR_FAMILY_IPV6 = 1,
} sai_ip_addr_family_t;

typedef struct _sai_ip_address_t {
    sai_ip_addr_family_t addr_family;
    union {
        uint32_t ip4;
        uint8_t ip6[16];
    } addr;
} sai_ip_address_t;

typedef struct _sai_ip_prefix_t {
    sai_ip_addr_family_t addr_family;
    union {
        uint32_t ip4;
        uint8_t ip6[16];
    } addr;
    union {
        uint32_t ip4;
        uint8_t ip6[16];
    } mask;
} sai_ip_prefix_t;

// --- SAI MAC ---
typedef uint8_t sai_mac_t[6];

// --- SAI Attribute Value ---
typedef union _sai_attribute_value_t {
    bool booldata;
    int32_t s32;
    uint32_t u32;
    uint64_t u64;
    sai_object_id_t oid;
    sai_ip_address_t ipaddr;
    sai_ip_prefix_t ipprefix;
    sai_mac_t mac;
} sai_attribute_value_t;

typedef struct _sai_attribute_t {
    uint32_t id;
    sai_attribute_value_t value;
} sai_attribute_t;

// --- SAI FDB Entry ---
typedef struct _sai_fdb_entry_t {
    sai_object_id_t switch_id;
    sai_mac_t mac_address;
    sai_object_id_t bv_id;  // Bridge/VLAN ID
} sai_fdb_entry_t;

// --- SAI Route Entry ---
typedef struct _sai_route_entry_t {
    sai_object_id_t switch_id;
    sai_object_id_t vr_id;  // Virtual router
    sai_ip_prefix_t destination;
} sai_route_entry_t;

// --- SAI FDB Attributes ---
typedef enum _sai_fdb_entry_attr_t {
    SAI_FDB_ENTRY_ATTR_TYPE = 0,
    SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID,
} sai_fdb_entry_attr_t;

// --- SAI Route Attributes ---
typedef enum _sai_route_entry_attr_t {
    SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID = 0,
    SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION,
} sai_route_entry_attr_t;

// --- SAI Next Hop Attributes ---
typedef enum _sai_next_hop_attr_t {
    SAI_NEXT_HOP_ATTR_TYPE = 0,
    SAI_NEXT_HOP_ATTR_IP,
    SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID,
} sai_next_hop_attr_t;

typedef enum _sai_next_hop_type_t {
    SAI_NEXT_HOP_TYPE_IP = 0,
} sai_next_hop_type_t;

// --- SAI Port Attributes ---
typedef enum _sai_port_attr_t {
    SAI_PORT_ATTR_ADMIN_STATE = 0,
    SAI_PORT_ATTR_SPEED,
    SAI_PORT_ATTR_OPER_STATUS,
} sai_port_attr_t;

// --- SAI API Function Tables ---
// (Defined in individual sai_*.h headers per object type)
