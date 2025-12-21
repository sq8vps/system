#ifndef KERNEL_STATUS_H_
#define KERNEL_STATUS_H_

/**
 * @brief Export all following lines up to the #END_EXPORT_API mark as kernel API calls
 */
#define EXPORT_API

/**
 * @brief End to-be-exported block started with #EXPORT_API
 */
#define END_EXPORT_API

EXPORT_API

/**
 * @brief Kernel status codes
*/
typedef enum
{
    OK = 0, /**< Everything is OK */

    BAD_PARAMETER, /**< Operation parameter is incorrect */
    NOT_IMPLEMENTED, /**< Operation is either explicitly not implemented or not supported regardless of other parameters */
    NOT_SUPPORTED, /**< Operation is not supported on given resource, probably because of other parameters */
    OUT_OF_RESOURCES, /**< Operation size is bigger than available resources, e.g., out of memory, buffer too small, list full, out of IDs. */
    DEVICE_NOT_AVAILABLE, /**< Device does not exist or is not available for given operation, e.g., because the owner changed */
    BAD_ALIGNMENT, /**< Bad memory alignment or offset */
    TIMEOUT, /**< Operation timed out */
    BUSY, /**< Resource is busy, e.g, there is an ongoing operation, resource cannot be shared, etc. */
    ALREADY_EXISTS, /**< Resource already exists */
    PAGE_NOT_PRESENT, /**< Page is not present in physical memory */
    CORRUPTED, /**< Resource exists, but is corrupted */
    UNDEFINED_SYMBOL, /**< Symbol is undefined */
    NOT_FOUND, /**< Resource not found */
    BAD_TYPE, /**< Bad object/resource/file/... type */
    FILE_CLOSED, /**< File is not open */
    RESOURCE_BOUND, /**< Resource is bound (to a peer, parent, or child) and can't be manipulated freely */
    READ_ONLY, /**< Resource is read-only */
    OPERATION_INCOMPLETE, /**< Operation finished but with incomplete data. This might or might not be an error */
    RESOURCE_PERSISTENT, /**< Resource is persistent and cannot be removed */

    UNKNOWN_ERROR = 0x1000, /**< Unknown or undefined error */
} STATUS;

END_EXPORT_API

#endif