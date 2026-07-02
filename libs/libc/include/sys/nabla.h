#ifndef __SYS_NABLA_H__
#define __SYS_NABLA_H__

/**
 * @brief Initialize C library
 * @param argc Argument count
 * @param **argv Argument value pointer table
 * @param **envp Environmental variables pointer table
 * @param *progData Program data table
 * @return >=0 on success, <0 on failure
 * @attention This function is normally called from \a crt0.o. If you need to skip crt0, call this function before entering main().
 */
int __nabla_init_libc(int argc, char **argv, char **envp, void *progData);

#endif /* __SYS_NABLA_H__ */
