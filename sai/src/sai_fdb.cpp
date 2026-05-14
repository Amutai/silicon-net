// silicon-net: SAI FDB Implementation
// TODO: Implement in Milestone 3
#include "sai.h"
#include "silicon_sdk.h"

sai_status_t sai_create_fdb_entry(const sai_fdb_entry_t* fdb_entry,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list) {
    // TODO: parse attrs, call silicon_sdk::fdb_add_entry
    return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_remove_fdb_entry(const sai_fdb_entry_t* fdb_entry) {
    // TODO
    return SAI_STATUS_NOT_SUPPORTED;
}
