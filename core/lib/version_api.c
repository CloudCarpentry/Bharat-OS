#include "bharat/version_api.h"
#include "bharat/component_version.h"

extern const struct bharat_component_version _bharat_component_info;

const struct bharat_component_version *bharat_get_component_info(void) {
    return &_bharat_component_info;
}
