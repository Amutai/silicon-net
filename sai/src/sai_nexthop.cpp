// silicon-net: SAI Next Hop Implementation
#include "sai.h"
#include "silicon_sdk.h"

// OID encoding: upper 16 bits = object type, lower 32 bits = SDK ID
static sai_object_id_t encode_oid(sai_object_type_t type, uint32_t id) {
    return (static_cast<uint64_t>(type) << 32) | id;
}

static uint32_t decode_sdk_id(sai_object_id_t oid) {
    return static_cast<uint32_t>(oid & 0xFFFFFFFF);
}

sai_status_t sai_create_next_hop(sai_object_id_t* next_hop_id,
                                 sai_object_id_t switch_id,
                                 uint32_t attr_count,
                                 const sai_attribute_t* attr_list) {
    if (!next_hop_id || !attr_list || attr_count == 0)
        return SAI_STATUS_INVALID_PARAMETER;

    silicon_sdk::NexthopEntry nh{};
    nh.egress_port = 0;
    nh.dst_mac = {0};

    for (uint32_t i = 0; i < attr_count; i++) {
        switch (attr_list[i].id) {
            case SAI_NEXT_HOP_ATTR_IP:
                nh.ip = attr_list[i].value.ipaddr.addr.ip4;
                break;
            case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
                nh.egress_port = static_cast<uint16_t>(attr_list[i].value.oid & 0xFFFF);
                break;
            default:
                break;
        }
    }

    silicon_sdk::NexthopId sdk_id;
    auto s = silicon_sdk::nexthop_create(&nh, &sdk_id);
    if (s == silicon_sdk::Status::ERR_TABLE_FULL) return SAI_STATUS_TABLE_FULL;
    if (s != silicon_sdk::Status::OK) return SAI_STATUS_FAILURE;

    *next_hop_id = encode_oid(SAI_OBJECT_TYPE_NEXT_HOP, sdk_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_remove_next_hop(sai_object_id_t next_hop_id) {
    uint32_t sdk_id = decode_sdk_id(next_hop_id);
    auto s = silicon_sdk::nexthop_remove(sdk_id);
    if (s == silicon_sdk::Status::ERR_NOT_FOUND) return SAI_STATUS_ITEM_NOT_FOUND;
    if (s == silicon_sdk::Status::ERR_IN_USE) return SAI_STATUS_OBJECT_IN_USE;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}
