// silicon-net: SAI ACL Implementation
#include "sai.h"
#include "silicon_sdk.h"

static sai_object_id_t encode_oid(sai_object_type_t type, uint32_t id) {
    return (static_cast<uint64_t>(type) << 32) | id;
}

static uint32_t decode_sdk_id(sai_object_id_t oid) {
    return static_cast<uint32_t>(oid & 0xFFFFFFFF);
}

sai_status_t sai_create_acl_entry(sai_object_id_t* acl_entry_id,
                                  sai_object_id_t switch_id,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list) {
    if (!acl_entry_id || !attr_list || attr_count == 0)
        return SAI_STATUS_INVALID_PARAMETER;

    silicon_sdk::AclRule rule{};
    rule.priority = 100;
    rule.action = silicon_sdk::AclAction::PERMIT;

    // Parse attributes — simplified for educational purposes
    // Real SAI has SAI_ACL_ENTRY_ATTR_FIELD_* and SAI_ACL_ENTRY_ATTR_ACTION_*
    // We use a minimal subset here

    silicon_sdk::AclRuleId sdk_id;
    auto s = silicon_sdk::acl_add_rule(&rule, &sdk_id);
    if (s == silicon_sdk::Status::ERR_TABLE_FULL) return SAI_STATUS_TABLE_FULL;
    if (s != silicon_sdk::Status::OK) return SAI_STATUS_FAILURE;

    *acl_entry_id = encode_oid(SAI_OBJECT_TYPE_ACL_ENTRY, sdk_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_remove_acl_entry(sai_object_id_t acl_entry_id) {
    uint32_t sdk_id = decode_sdk_id(acl_entry_id);
    auto s = silicon_sdk::acl_remove_rule(sdk_id);
    if (s == silicon_sdk::Status::ERR_NOT_FOUND) return SAI_STATUS_ITEM_NOT_FOUND;
    return (s == silicon_sdk::Status::OK) ? SAI_STATUS_SUCCESS : SAI_STATUS_FAILURE;
}
