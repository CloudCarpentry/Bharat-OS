/*
 * Capability subsystem facade.
 *
 * Implementation has been modularized into:
 *   - core/kernel/src/cap/cap_cspace.c   (CSpace lifecycle, locators, slot allocation)
 *   - core/kernel/src/cap/cap_dispatch.c (Validation, typed lookup, authority resolver)
 *   - core/kernel/src/cap/cap_revoke.c   (Tree walk, sorted locking, cross-core revoke)
 */

#include "capability.h"
#include "cap/cap_internal.h"
