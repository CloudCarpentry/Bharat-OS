#ifndef BHARAT_INIT_GRAPH_H
#define BHARAT_INIT_GRAPH_H

#include "init_manifest.h"
#include "init_status.h"
#include <bharat/uapi/init/init_boot_context.h>

typedef enum {
    INIT_GRAPH_OK = 0,
    INIT_GRAPH_ERR_DUPLICATE_SERVICE,
    INIT_GRAPH_ERR_UNKNOWN_DEP,
    INIT_GRAPH_ERR_FILTERED_DEP,
    INIT_GRAPH_ERR_CYCLE,
    INIT_GRAPH_ERR_NO_CORE_SERVICE,
    INIT_GRAPH_ERR_REQUIRED_CAP_MISSING,
} init_graph_result_t;

/**
 * Validates the service manifest graph.
 *
 * @param manifest Pointer to the array of services (already filtered or full list).
 * @param count Number of services in the manifest.
 * @param ctx The boot context.
 * @return init_graph_result_t
 */
init_graph_result_t init_graph_validate(const init_service_desc_t *manifest,
                                        size_t count,
                                        const init_boot_context_t *ctx);

/**
 * Maps graph error codes to init failure classes.
 */
init_failure_class_t init_graph_failure_to_init_failure(init_graph_result_t rc);

#endif // BHARAT_INIT_GRAPH_H
