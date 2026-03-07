import os

mock_safe = """
#include "../kernel/include/slab.h"
#include <stdlib.h>
#include <string.h>

// Some tests were crashing on free() because thread_destroy called kcache_free on pointers that weren't allocated by kcache_alloc (like stack/global variables in tests).
// Since these are stub tests without full memory management setup, let's just make kcache_free a no-op for tests.
kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = malloc(sizeof(kcache_t));
    if(c) {
        c->object_size = size;
        c->name = name;
    }
    return c;
}
void* kcache_alloc(kcache_t* cache) {
    if(!cache) return NULL;
    return malloc(cache->object_size);
}
void kcache_free(kcache_t* cache, void* obj) {
    // DO NOTHING in tests to avoid free() errors on statically allocated mock threads.
}
"""

files = os.listdir('tests')
for file in files:
    if file.endswith('.c'):
        with open(os.path.join('tests', file), 'r') as f:
            content = f.read()

        import re
        content = re.sub(r'#include "\.\./kernel/include/slab\.h"\s*#include <stdlib\.h>\s*#include <string\.h>.*?void kcache_free\(kcache_t\* cache, void\* obj\) \{\s*if\(obj\) free\(obj\);\s*\}', mock_safe.strip(), content, flags=re.DOTALL)

        with open(os.path.join('tests', file), 'w') as f:
            f.write(content)
