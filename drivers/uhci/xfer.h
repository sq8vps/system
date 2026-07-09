#ifndef UHCI_XFER_H_
#define UHCI_XFER_H_

#include "defines.h"

struct UhciControllerInfo;

/**
 * @brief Allocate empty frame list for HC
 * @param *info HC info structure
 * @return Status code
 */
STATUS UhciAllocateFrameList(struct UhciControllerInfo *info);

#endif