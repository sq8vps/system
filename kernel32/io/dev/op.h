#ifndef DEV_OP_H_
#define DEV_OP_H_

#include "defines.h"
#include <stdbool.h>

EXPORT
struct IoDeviceObject;
struct IoVfsNode;

EXPORT
typedef void (*IoReadWriteCompletionCallback)(STATUS status, uint64_t actualSize, void *context);

STATUS IoReadWrite(bool write, struct IoDeviceObject *dev, struct IoVfsNode *node, uint64_t offset, uint64_t size, void *buffer,
                IoReadWriteCompletionCallback callback, void *context, bool forceDirectIo);

EXPORT
/**
 * @brief Perfrom synchronous device read
 * 
 * This function performs direct I/O read if possible in the first place.
 * If offset alignment is not compliant with device block size and alignment requirements,
 * then the double buffering is used. Otherwise a direct read is performed.
 * The same restrictions apply when buffer is already provided by the caller.
 * @param *dev Device object to perform the read on
 * @param offset Offset to start reading from
 * @param size Size of data to read
 * @param **buffer Output buffer pointer, allocated by this function
 * @return Status code
*/
EXTERN STATUS IoReadDeviceSync(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void **buffer);

#endif