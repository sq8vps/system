#ifndef ARCH_I686_H_
#define ARCH_I686_H_

#define I686_VERIFY_INTERRUPT_VECTOR(x) (((x) > 32) && ((x) < 256))
#define I686_INVALID_INTERRUPT_VECTOR 0

#endif