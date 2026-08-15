#ifndef BHARAT_FS_PIPE_H
#define BHARAT_FS_PIPE_H

#include "fs/vfs.h"

int fs_pipe_create(vfs_node_t** out_read_node, vfs_node_t** out_write_node);

#endif // BHARAT_FS_PIPE_H
