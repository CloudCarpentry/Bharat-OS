#include "../../include/numa.h"

int numa_discover_topology(void) {
    return 0;
}

memory_node_id_t numa_get_current_node(void) {
    return NUMA_NODE_LOCAL;
}
