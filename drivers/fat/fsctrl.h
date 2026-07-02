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

/**
 * @brief Get node or node children from FAT volume asynchronously
 * @param *rp Request Packet
 * @param *vol FAT volume structure
 * @return Status code
 */
STATUS FatGetNode(struct IoRp *rp, struct FatVolume *vol);

/**
 * @brief Create a new file
 * @param *rp Request Packet
 * @param *vol FAT volume structure
 * @return Status code
 */
STATUS FatCreateFile(struct IoRp *rp, struct FatVolume *vol);

#endif