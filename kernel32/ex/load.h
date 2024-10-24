#ifndef KERNEL_BOOTSTRAP_H_
#define KERNEL_BOOTSTRAP_H_

#include <stdint.h>
#include "defines.h"

INTERNAL STATUS ExLoadProcessImage(const char *path, void (**entry)());

#endif