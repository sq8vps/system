#include "sys/unistd.h"
#include "ke/task/task.h"
#include "sys/errno.h"

pid_t _getpid(void)
{
    return ApiGetTid();
}

[[noreturn]] void _exit (int __status)
{
    ApiExitTask(__status);
}

pid_t _wait(int *wstatus)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement wait()
}

int _execve(const char *__path, char * const __argv[], char * const __envp[])
{
    errno = ENOSYS;
    return -1;
    //TODO: implement execve() 
}

int _fork(void)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement fork() 
}

int _kill(pid_t pid, int sig)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement kill() 
}