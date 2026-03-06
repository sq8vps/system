#include "common.h"
#include "defines.h"
#include "ke/sys/syscall.h"
#include "ke/task/task.h"
#include "io/fs/fs.h"
#include "mm/tmem.h"
#include "ddk/tty.h"

reg_t __ApiDoSyscall(reg_t argSize, reg_t code, ...);

SYSCALL_WRAPPER(STATUS, ApiNoOperation, uint32_t, uint16_t, uint8_t, void*, uint64_t);

[[noreturn]] void ApiExitTask(int status)
{
    __ApiDoSyscall(sizeof(int), SysApiExitTask, status);
    while(1)
        ;
}

SYSCALL_WRAPPER(STATUS, ApiSetThreadLocalStorage, void*);

SYSCALL_WRAPPER(STATUS, ApiOpenFile, const char*, IoFileOpenMode, IoFileFlags, int*);
SYSCALL_WRAPPER(STATUS, ApiCloseFile, int);
SYSCALL_WRAPPER(STATUS, ApiReadFileSync, int, void*, size_t, uint64_t, size_t*);
SYSCALL_WRAPPER(STATUS, ApiWriteFileSync, int, void*, size_t, uint64_t, size_t*);


SYSCALL_WRAPPER(STATUS, ApiSymlink, const char *, const char *);

SYSCALL_WRAPPER(STATUS, ApiMapTaskMemory, void*, size_t, enum MmTaskMemoryFlags, int, size_t, uint64_t, size_t, void**);
SYSCALL_WRAPPER(STATUS, ApiMapTaskMemoryA, void*, size_t, enum MmTaskMemoryFlags, void**);
SYSCALL_WRAPPER(STATUS, ApiUnmapTaskMemory, const void *const, size_t);

SYSCALL_WRAPPER(STATUS, ApiGetSystemConfig, enum ApiSystemConfigParam, uint64_t*);

SYSCALL_WRAPPER(STATUS, ApiCreateVt, int, int, int, char*);

