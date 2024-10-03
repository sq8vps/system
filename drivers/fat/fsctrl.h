#ifndef FAT_FSCTRL_H_
#define FAT_FSCTRL_H_

#include "defines.h"

struct IoRp;
struct FatVolume;
struct IoVfsNode;

/**
 * @brief Dispatch FS control request
 * @param *rp Request Packet
 * @return Status code
 */
STATUS FatFsControl(struct IoRp *rp);

/**
 * @brief Update attributes for given file
 * @param *vol FAT volume structure
 * @param *node Corresponding VFS node
 * @return Status code
 */
STATUS FatUpdateFileAttributes(struct FatVolume *vol, struct IoVfsNode *node);

#endif