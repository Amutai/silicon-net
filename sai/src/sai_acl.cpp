// silicon-net: SAI ACL Implementation
// TODO: Implement in Milestone 3
#include "sai.h"
#include "silicon_sdk.h"

sai_status_t sai_create_acl_entry(sai_object_id_t* acl_entry_id,
                                  sai_object_id_t switch_id,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list) {
    // TODO: parse attrs, call silicon_sdk::acl_add_rule
    return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_remove_acl_entry(sai_object_id_t acl_entry_id) {
    // TODO
    return SAI_STATUS_NOT_SUPPORTED;
}
