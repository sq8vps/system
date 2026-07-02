#ifndef HELPERS_H_
#define HELPERS_H_

#include "defines.h"
#include "sys/errno.h"

/**
 * @brief Convert Nabla kernel status to errno
 * @param status Kernel status
 * @return \a errno value corresponding to \a status
 */
int NablaStatusToErrno(STATUS status);

/**
 * @brief Return from libc function and set errno if needed
 * @param status Kernel status
 * @param onSuccess Value to return on success
 */
#define __NABLA_RETURN(status, onSuccess) return (OK == (status)) ? (onSuccess) : (errno = NablaStatusToErrno(status), -1)

#endif