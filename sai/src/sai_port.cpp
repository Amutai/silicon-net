// silicon-net: SAI Port Implementation
#include "sai.h"
#include "silicon_sdk.h"

// SAI port OID encoding: lower 16 bits = port index
static uint16_t oid_to_port(sai_object_id_t oid) {
    return static_cast<uint16_t>(oid & 0xFFFF);
}

sai_status_t sai_set_port_attribute(sai_object_id_t port_id,
                                    const sai_attribute_t* attr) {
    if (!attr) return SAI_STATUS_INVALID_PARAMETER;
    uint16_t port = oid_to_port(port_id);

    switch (attr->id) {
        case SAI_PORT_ATTR_ADMIN_STATE: {
            auto s = silicon_sdk::port_set_admin_state(port, attr->value.booldata);
            if (s == silicon_sdk::Status::ERR_INVALID_PARAM)
                return SAI_STATUS_INVALID_PARAMETER;
            return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
        }
        default:
            return SAI_STATUS_NOT_SUPPORTED;
    }
}

sai_status_t sai_get_port_attribute(sai_object_id_t port_id,
                                    uint32_t attr_count,
                                    sai_attribute_t* attr_list) {
    if (!attr_list || attr_count == 0) return SAI_STATUS_INVALID_PARAMETER;
    uint16_t port = oid_to_port(port_id);

    for (uint32_t i = 0; i < attr_count; i++) {
        switch (attr_list[i].id) {
            case SAI_PORT_ATTR_ADMIN_STATE: {
                bool state = false;
                auto s = silicon_sdk::port_get_admin_state(port, &state);
                if (s != silicon_sdk::Status::OK) return SAI_STATUS_INVALID_PARAMETER;
                attr_list[i].value.booldata = state;
                break;
            }
            default:
                return SAI_STATUS_NOT_SUPPORTED;
        }
    }
    return SAI_STATUS_SUCCESS;
}
