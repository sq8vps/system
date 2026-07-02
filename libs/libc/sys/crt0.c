

#include "stdlib.h"
#include "sys/nabla.h"
#include "ke/task/task.h"

extern int main(int argc, char **argv, char **envp, void *progData);

void _start(int argc, char **argv, char **envp, void *progData)
{
    int rc = __nabla_init_libc(argc, argv, envp, progData);
    if(rc < 0)
        ApiExitTask(rc);

    exit(main(argc, argv, envp, progData));
}