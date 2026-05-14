// silicon-net: SAI Next Hop Implementation
// TODO: Implement in Milestone 3
#include "sai.h"
#include "silicon_sdk.h"

sai_status_t sai_create_next_hop(sai_object_id_t* next_hop_id,
                                 sai_object_id_t switch_id,
                                 uint32_t attr_count,
                                 const sai_attribute_t* attr_list) {
    // TODO: parse attrs, call silicon_sdk::nexthop_create
    return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_remove_next_hop(sai_object_id_t next_hop_id) {
    // TODO
    return SAI_STATUS_NOT_SUPPORTED;
}
