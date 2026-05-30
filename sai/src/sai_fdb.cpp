// silicon-net: SAI FDB Implementation
#include "sai.h"
#include "silicon_sdk.h"
#include <cstring>

sai_status_t sai_create_fdb_entry(const sai_fdb_entry_t* fdb_entry,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list) {
    if (!fdb_entry || !attr_list || attr_count == 0)
        return SAI_STATUS_INVALID_PARAMETER;

    uint16_t port_id = 0;
    bool has_port = false;

    for (uint32_t i = 0; i < attr_count; i++) {
        if (attr_list[i].id == SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID) {
            port_id = static_cast<uint16_t>(attr_list[i].value.oid & 0xFFFF);
            has_port = true;
        }
    }

    if (!has_port) return SAI_STATUS_INVALID_PARAMETER;

    silicon_sdk::FdbEntry entry{};
    std::memcpy(entry.mac.data(), fdb_entry->mac_address, 6);
    entry.vlan = static_cast<uint16_t>(fdb_entry->bv_id & 0xFFFF);
    entry.port = port_id;

    auto s = silicon_sdk::fdb_add_entry(&entry);
    if (s == silicon_sdk::Status::ERR_TABLE_FULL) return SAI_STATUS_TABLE_FULL;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}

sai_status_t sai_remove_fdb_entry(const sai_fdb_entry_t* fdb_entry) {
    if (!fdb_entry) return SAI_STATUS_INVALID_PARAMETER;

    silicon_sdk::MacAddr mac{};
    std::memcpy(mac.data(), fdb_entry->mac_address, 6);
    uint16_t vlan = static_cast<uint16_t>(fdb_entry->bv_id & 0xFFFF);

    auto s = silicon_sdk::fdb_remove_entry(mac, vlan);
    if (s == silicon_sdk::Status::ERR_NOT_FOUND) return SAI_STATUS_ITEM_NOT_FOUND;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}
