#include "backend_dispatch.h"
#include <stddef.h>

#define MAX_BACKENDS 32

static const backend_provider_t *g_registry[MAX_BACKENDS];
static uint32_t g_registry_count = 0;

#include <string.h>

int backend_registry_add(const backend_provider_t *provider) {
    if (!provider || !provider->name) return -1;
    if (g_registry_count >= MAX_BACKENDS) return -1;

    // Reject duplicate provider identity/name to prevent ordering/telemetry ambiguity
    for (uint32_t i = 0; i < g_registry_count; i++) {
        if (strcmp(g_registry[i]->name, provider->name) == 0) {
            return -2;
        }
    }

    g_registry[g_registry_count++] = provider;
    return 0;
}

void backend_registry_reset(void) {
    g_registry_count = 0;
}

const backend_provider_t *backend_dispatch_select(
    bharat_feature_class_t feature_class,
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx)
{
    if (!caps || !ctx) return NULL;

    const backend_provider_t *best_hw = NULL;
    const backend_provider_t *best_sw = NULL;

    for (uint32_t i = 0; i < g_registry_count; i++) {
        const backend_provider_t *p = g_registry[i];

        // Filter by requested feature class
        if (p->feature_class != feature_class) continue;

        // Ensure the backend claims it is available in this context
        if (p->is_available && !p->is_available(caps, ctx)) continue;

        if (p->type == BACKEND_TYPE_SOFTWARE_FALLBACK) {
            if (!best_sw || p->priority > best_sw->priority) {
                best_sw = p;
            }
        } else {
            // Hardware backends are usually skipped if we are in strict safe mode
            // or extremely constrained power modes that dictate SW only, but we let
            // the `is_available` callback handle most of those constraints.
            if (!best_hw || p->priority > best_hw->priority) {
                best_hw = p;
            }
        }
    }

    // Respect preferred backend from exec plan if specified
    if (ctx->preferred_backend == 1 /* BACKEND_NPU */) {
        if (best_hw && !ctx->safe_mode) return best_hw;
        if (!ctx->fallback_allowed) return NULL;
        return best_sw;
    } else if (ctx->preferred_backend == 0 /* BACKEND_CPU */ || ctx->preferred_backend == 2 /* BACKEND_SAFE */) {
        if (best_sw) return best_sw;
        if (!ctx->fallback_allowed) return NULL;
    }

    // Prefer hardware unless constrained, else fallback to software
    if (!ctx->fallback_allowed && !best_hw) {
        return NULL;
    }

    if (best_hw && !ctx->safe_mode) {
        return best_hw;
    }

    return best_sw;
}
