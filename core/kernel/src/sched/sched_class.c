#include <sched/sched_class.h>
#include <stddef.h>

#define MAX_SCHED_CLASSES 8

static sched_class_ops_t* g_sched_classes[MAX_SCHED_CLASSES] = {0};
static int g_num_classes = 0;

void sched_class_registry_init(void) {
    g_num_classes = 0;
    for (int i = 0; i < MAX_SCHED_CLASSES; i++) {
        g_sched_classes[i] = NULL;
    }
}

int sched_class_register(sched_class_ops_t *ops) {
    if (!ops || !ops->name || ops->class_mask == BHARAT_SCHED_CLASS_NONE) {
        return -1;
    }

    if (g_num_classes >= MAX_SCHED_CLASSES) {
        return -2; // No space
    }

    // Check for duplicate mask
    for (int i = 0; i < g_num_classes; i++) {
        if (g_sched_classes[i] && g_sched_classes[i]->class_mask == ops->class_mask) {
            return -3; // Duplicate mask
        }
    }

    g_sched_classes[g_num_classes++] = ops;
    return 0;
}

sched_class_ops_t* sched_class_find_by_mask(bharat_sched_class_mask_t mask) {
    for (int i = 0; i < g_num_classes; i++) {
        if (g_sched_classes[i] && (g_sched_classes[i]->class_mask & mask)) {
            // Note: If multiple bits are set, it returns the first one that matches any bit.
            // Ideally callers query for a specific class or we handle iteration separately.
            return g_sched_classes[i];
        }
    }
    return NULL;
}
