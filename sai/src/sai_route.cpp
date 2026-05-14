// silicon-net: SAI Route Implementation
// TODO: Implement in Milestone 3
#include "sai.h"
#include "silicon_sdk.h"

sai_status_t sai_create_route_entry(const sai_route_entry_t* route_entry,
                                    uint32_t attr_count,
                                    const sai_attribute_t* attr_list) {
    // TODO: parse attrs, call silicon_sdk::route_add
    return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_remove_route_entry(const sai_route_entry_t* route_entry) {
    // TODO
    return SAI_STATUS_NOT_SUPPORTED;
}
