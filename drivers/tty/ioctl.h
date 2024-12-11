#ifndef TTY_IOCTL_H_
#define TTY_IOCTL_H_

#include "defines.h"

struct IoRp;

/**
 * @brief Generic terminal driver IOCTL codes
 */
enum TtyIoctlCodes
{
    TTY_IOCTL_NONE = 0, /**< Dummy IOCTL. Always successful. */
    TTY_IOCTL_CREATE_VT = 1, /**< Create virtual terminal device with default settings.
        The TTY ID is returned through the IOCTL payload. The created device is associated with
        /dev/tty<n> file, where <n> is the returned ID. */
    TTY_IOCTL_ACTIVATE = 2, /**< Activate terminal */
};

STATUS TtyHandleIoctl(struct IoRp *rp);

#endif