/*
 * Kernel algorithm dispatch is intentionally self-contained. Reusable-library
 * backends are selected by that library's runtime and are never registered by
 * architecture or kernel initialization code.
 */
void bharat_algorithm_backends_init(void) {}
