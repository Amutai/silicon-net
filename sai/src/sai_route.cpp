// silicon-net: SAI Route Implementation
#include "sai.h"
#include "silicon_sdk.h"

static uint32_t decode_sdk_id(sai_object_id_t oid) {
    return static_cast<uint32_t>(oid & 0xFFFFFFFF);
}

static uint8_t mask_to_prefix_len(uint32_t mask) {
    uint8_t len = 0;
    while (mask & 0x80000000) {
        len++;
        mask <<= 1;
    }
    return len;
}

sai_status_t sai_create_route_entry(const sai_route_entry_t* route_entry,
                                    uint32_t attr_count,
                                    const sai_attribute_t* attr_list) {
    if (!route_entry || !attr_list || attr_count == 0)
        return SAI_STATUS_INVALID_PARAMETER;

    sai_object_id_t nh_oid = SAI_NULL_OBJECT_ID;
    for (uint32_t i = 0; i < attr_count; i++) {
        if (attr_list[i].id == SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID)
            nh_oid = attr_list[i].value.oid;
    }

    if (nh_oid == SAI_NULL_OBJECT_ID) return SAI_STATUS_INVALID_PARAMETER;

    silicon_sdk::RouteEntry entry{};
    entry.prefix = route_entry->destination.addr.ip4;
    entry.prefix_len = mask_to_prefix_len(route_entry->destination.mask.ip4);
    entry.nexthop_id = decode_sdk_id(nh_oid);

    auto s = silicon_sdk::route_add(&entry);
    if (s == silicon_sdk::Status::ERR_TABLE_FULL) return SAI_STATUS_TABLE_FULL;
    if (s == silicon_sdk::Status::ERR_NOT_FOUND) return SAI_STATUS_ITEM_NOT_FOUND;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}

sai_status_t sai_remove_route_entry(const sai_route_entry_t* route_entry) {
    if (!route_entry) return SAI_STATUS_INVALID_PARAMETER;

    uint32_t prefix = route_entry->destination.addr.ip4;
    uint8_t prefix_len = mask_to_prefix_len(route_entry->destination.mask.ip4);

    auto s = silicon_sdk::route_remove(prefix, prefix_len);
    if (s == silicon_sdk::Status::ERR_NOT_FOUND) return SAI_STATUS_ITEM_NOT_FOUND;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}
