#include "fs/vfs.h"
#include <string.h>

void split_path(const char* full_path, char* parent_path, char* leaf_name, size_t max_len) {
    if (!full_path || !parent_path || !leaf_name) return;

    size_t len = 0;
    while(full_path[len] != '\0') len++;

    // Find last slash
    size_t last_slash = len;
    for (size_t i = len; i > 0; i--) {
        if (full_path[i - 1] == '/') {
            last_slash = i - 1;
            break;
        }
    }

    if (last_slash == len) {
        // No slash found, meaning it's in root or current dir
        parent_path[0] = '/';
        parent_path[1] = '\0';
        size_t i = 0;
        while(full_path[i] != '\0' && i < max_len - 1) {
            leaf_name[i] = full_path[i];
            i++;
        }
        leaf_name[i] = '\0';
    } else {
        // Slash found
        size_t i = 0;
        if (last_slash == 0) {
            parent_path[0] = '/';
            parent_path[1] = '\0';
        } else {
            while(i < last_slash && i < max_len - 1) {
                parent_path[i] = full_path[i];
                i++;
            }
            parent_path[i] = '\0';
        }

        i = 0;
        size_t j = last_slash + 1;
        while(full_path[j] != '\0' && i < max_len - 1) {
            leaf_name[i] = full_path[j];
            i++;
            j++;
        }
        leaf_name[i] = '\0';
    }
}
