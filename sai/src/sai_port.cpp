// silicon-net: SAI Port Implementation
// TODO: Implement in Milestone 3
#include "sai.h"
#include "silicon_sdk.h"

sai_status_t sai_set_port_attribute(sai_object_id_t port_id,
                                    const sai_attribute_t* attr) {
    // TODO: map SAI port attrs to SDK calls
    return SAI_STATUS_NOT_SUPPORTED;
}

sai_status_t sai_get_port_attribute(sai_object_id_t port_id,
                                    uint32_t attr_count,
                                    sai_attribute_t* attr_list) {
    // TODO
    return SAI_STATUS_NOT_SUPPORTED;
}
