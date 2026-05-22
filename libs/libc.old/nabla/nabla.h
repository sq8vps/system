#ifndef NABLA_LIBC_NABLA_H_
#define NABLA_LIBC_NABLA_H_

#include <stddef.h>
#include "state.h"

/**
 * @brief Initialize C library from a new thread
 */
void __nabla_libc_initialize(void);

/**
 * @brief Create Thread-local storage and prepare heap allocator
 * @return Created TLS pointer or NULL on failure
 */
void *_nabla_create_tls(void);

/**
 * @brief Get Thread-local Storage pointer
 * @return TLS pointer
 */
struct _nabla_libc_thread_state *_nabla_get_tls(void);

/**
 * @brief Set new Thread-local Storage pointer in kernel
 * @param *tls TLS pointer
 */
void _nabla_set_tls(struct _nabla_libc_thread_state *tls);

/**
 * @brief Obtain errno pointer
 * @return Errno pointer
 */
int *__errno(void);



#endif