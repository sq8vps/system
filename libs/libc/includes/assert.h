#ifndef NABLA_LIBC_ASSERT_H
#define NABLA_LIBC_ASSERT_H

#define __STDC_VERSION_ASSERT_H__ 202311L

#ifdef NDEBUG
#define assert(...) ((void)0)
#else
#define assert(...) ((void)0) 
//TODO: implement assert()
#endif

#endif