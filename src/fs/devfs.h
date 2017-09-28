/*
 * fs/devfs.h
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief Device fs definitions
 */

#include <osZ.h>

#define DEVPATH "/dev/"

#define MEMFS_MAJOR      0
#define MEMFS_ZERO       0
#define MEMFS_RAMDISK    1
#define MEMFS_RANDOM     2
#define MEMFS_NULL       3
#define MEMFS_TMPFS      4

/* device type */
typedef struct {
    fid_t fid;          //name in fcb
    pid_t drivertask;   //major
    dev_t device;       //minor
    blksize_t blksize;
    blkcnt_t blkcnt;
} device_t;

extern uint64_t ndev;
extern device_t *dev;

extern void devfs_init();
extern uint64_t defs_add();

#if DEBUG
extern void devfs_dump();
#endif
