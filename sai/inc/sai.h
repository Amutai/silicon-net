#pragma once
// silicon-net: SAI Top-Level Header
// Simplified version of the OCP SAI interface.

#include "saitypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Switch API ---
sai_status_t sai_create_switch(sai_object_id_t* switch_id,
                               uint32_t attr_count,
                               const sai_attribute_t* attr_list);
sai_status_t sai_remove_switch(sai_object_id_t switch_id);

// --- Port API ---
sai_status_t sai_set_port_attribute(sai_object_id_t port_id,
                                    const sai_attribute_t* attr);
sai_status_t sai_get_port_attribute(sai_object_id_t port_id,
                                    uint32_t attr_count,
                                    sai_attribute_t* attr_list);

// --- FDB API ---
sai_status_t sai_create_fdb_entry(const sai_fdb_entry_t* fdb_entry,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list);
sai_status_t sai_remove_fdb_entry(const sai_fdb_entry_t* fdb_entry);

// --- Route API ---
sai_status_t sai_create_route_entry(const sai_route_entry_t* route_entry,
                                    uint32_t attr_count,
                                    const sai_attribute_t* attr_list);
sai_status_t sai_remove_route_entry(const sai_route_entry_t* route_entry);

// --- Next Hop API ---
sai_status_t sai_create_next_hop(sai_object_id_t* next_hop_id,
                                 sai_object_id_t switch_id,
                                 uint32_t attr_count,
                                 const sai_attribute_t* attr_list);
sai_status_t sai_remove_next_hop(sai_object_id_t next_hop_id);

// --- ACL API ---
sai_status_t sai_create_acl_entry(sai_object_id_t* acl_entry_id,
                                  sai_object_id_t switch_id,
                                  uint32_t attr_count,
                                  const sai_attribute_t* attr_list);
sai_status_t sai_remove_acl_entry(sai_object_id_t acl_entry_id);

#ifdef __cplusplus
}
#endif
