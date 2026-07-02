#ifndef FAT_RW_H_
#define FAT_RW_H_

#include "defines.h"

struct IoRp;

/**
 * @brief Process read or write request
 * @param *rp Request Packet
 * @return Status code
 */
STATUS FatReadWrite(struct IoRp *rp);

#endif