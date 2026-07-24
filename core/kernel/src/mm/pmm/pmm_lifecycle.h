#ifndef BHARAT_MM_PMM_LIFECYCLE_H
#define BHARAT_MM_PMM_LIFECYCLE_H

#include "mm.h"
#include "kernel/status.h"
#include <stdbool.h>

/**
 * @file pmm_lifecycle.h
 * @brief Internal PMM page lifecycle management.
 */

/**
 * @brief Validate and perform a state transition for a page.
 * @param page Pointer to the page metadata.
 * @param expected The expected current state.
 * @param next The target next state.
 * @return K_OK on success, K_ERR_BAD_STATE if transition is invalid.
 */
kstatus_t pmm_page_transition(page_t *page, pmm_page_state_t expected, pmm_page_state_t next);

/**
 * @brief Check if a page can be safely freed.
 * @param page Pointer to the page metadata.
 * @return True if it can be freed (not pinned, etc.).
 */
bool pmm_page_can_free(const page_t *page);

/**
 * @brief Check if a page is currently allocatable.
 * @param page Pointer to the page metadata.
 * @return True if it is in FREE state.
 */
bool pmm_page_is_allocatable(const page_t *page);

/**
 * @brief Check if a page is pinned (e.g., for DMA).
 * @param page Pointer to the page metadata.
 * @return True if pinned.
 */
bool pmm_page_is_pinned(const page_t *page);

/**
 * @brief Safe wrapper to increment page refcount.
 * @param page Pointer to the page metadata.
 * @return K_OK on success.
 */
kstatus_t pmm_page_get(page_t *page);

/**
 * @brief Safe wrapper to decrement page refcount.
 * @param page Pointer to the page metadata.
 * @param is_last Set to true if this was the last reference.
 * @return K_OK on success.
 */
kstatus_t pmm_page_put(page_t *page, bool *is_last);

#endif // BHARAT_MM_PMM_LIFECYCLE_H
