#ifndef UHCI_XFER_H_
#define UHCI_XFER_H_

#include "defines.h"

struct UhciControllerInfo;

struct UhciTransfer
{

};

/**
 * @brief Allocate structures for HC
 * @param *info HC info structure
 * @return Status code
 */
STATUS UhciAllocateStructures(struct UhciControllerInfo *info);

#endif