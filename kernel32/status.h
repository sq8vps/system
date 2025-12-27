/**
 * @file status.h
 * @brief Kernel status codes
 * @ingroup defines
 */

#ifndef KERNEL_STATUS_H_
#define KERNEL_STATUS_H_

/**
 * @addtogroup defines
 * @{
 */

/**
 * @brief Mark all following lines up to the #END_EXPORT_API mark as kernel API calls
 */
#define EXPORT_API

/**
 * @brief End to-be-exported block started with #EXPORT_API
 */
#define END_EXPORT_API

EXPORT_API

/**
 * @brief Kernel status codes
 * @note Function return these values directly - as positive integers
*/
typedef enum STATUS
{
    OK = 0, /**< Everything is OK */

    BAD_PARAMETER = 1, /**< Operation parameter is incorrect */
    NOT_IMPLEMENTED = 2, /**< Operation is either explicitly not implemented or not supported regardless of other parameters */
    NOT_SUPPORTED = 3, /**< Operation is not supported on given resource, probably because of other parameters */
    OUT_OF_RESOURCES = 4, /**< Operation size is bigger than available resources, e.g., out of memory, buffer too small, list full, out of IDs. */
    DEVICE_NOT_AVAILABLE = 5, /**< Device does not exist or is not available for given operation, e.g., because the owner changed */
    BAD_ALIGNMENT = 6, /**< Bad memory alignment or offset */
    TIMEOUT = 7, /**< Operation timed out */
    BUSY = 8, /**< Resource is busy, e.g, there is an ongoing operation, resource cannot be shared, etc. */
    ALREADY_EXISTS = 9, /**< Resource already exists */
    PAGE_NOT_PRESENT = 10, /**< Page is not present in physical memory */
    CORRUPTED = 11, /**< Resource exists, but is corrupted */
    UNDEFINED_SYMBOL = 12, /**< Symbol is undefined */
    NOT_FOUND = 13, /**< Resource not found */
    BAD_TYPE = 14, /**< Bad object/resource/file/... type */
    FILE_CLOSED = 15, /**< File is not open */
    RESOURCE_BOUND = 16, /**< Resource is bound (to a peer, parent, or child) and can't be manipulated freely */
    READ_ONLY = 17, /**< Resource is read-only */
    OPERATION_INCOMPLETE = 18, /**< Operation finished but with incomplete data. This might or might not be an error */
    RESOURCE_PERSISTENT = 19, /**< Resource is persistent and cannot be removed */

    UNKNOWN_ERROR = 0x1000, /**< Unknown or undefined error */
} STATUS;

END_EXPORT_API

/**
 * @}
 */

#endif