
#include <stdio.h>
#include <ddk/tty.h>
#include <io/fs/fs.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

constexpr char DefaultVtMaster[] = "/dev/ttyM0";
constexpr int DefaultVtInput = 0;
constexpr int DefaultVtOutput = 0;

//constexpr char DefaultTty[]= "/dev/tty0";

static STATUS CreateFileDescriptors(void)
{
    STATUS status = OK;
    status = ApiSymlink("/dev/stdin", "/task/self/fd/" STRINGIFY(STDIN_FILENO));
    if(OK != status)
        return status;
    status = ApiSymlink("/dev/stdout", "/task/self/fd/" STRINGIFY(STDOUT_FILENO));
    if(OK != status)
        return status;
    status = ApiSymlink("/dev/stderr", "/task/self/fd/" STRINGIFY(STDERR_FILENO));
    if(OK != status)
        return status;

    return OK;
}

static STATUS PrepareStdHandles(const char *device)
{
    STATUS status = OK;
    int fd[3] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    
    status = ApiOpenFile(device, IO_FILE_READ | IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[0]);
    if(OK != status)
        return status;
    status = ApiOpenFile(device, IO_FILE_READ | IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[1]);
    if(OK != status)
    {
        ApiCloseFile(fd[0]);
        return status;
    }
    status = ApiOpenFile(device, IO_FILE_READ | IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[2]);
    if(OK != status)
    {
        ApiCloseFile(fd[0]);
        ApiCloseFile(fd[1]);
        return status;
    }

    if(OK != CreateFileDescriptors())
    {
        ApiCloseFile(fd[0]);
        ApiCloseFile(fd[1]);
        ApiCloseFile(fd[2]);
        return false;
    }

    return true;
}

bool OpenVt(int argc, char **argv)
{
    STATUS status = OK;
    bool useDefaults = true;
    const char *masterPath = nullptr;
    int master = -1;
    int input = -1;
    int output = -1;
    char name[5 + TTY_DEVICE_NAME_SIZE + 1] = "/dev/";

    for(int i = 1; i < argc; i++)
    {
        if((nullptr != argv[i]) && (argv[i] == strstr(argv[i], "vt=")))
        {
            char *next1, *next2;
            masterPath = argv[i] + 3;
            
            next1 = strpbrk(masterPath, ",");
            if(nullptr == next1)
                break;
            *next1 = '\0';
            ++next1;
            errno = 0;
            output = strtol(next1, &next2, 10);
            if((0 != errno) || (next1 == next2) || (',' != *next2))
                break;
            ++next2;
            errno = 0;
            input = strtol(next2, &next1, 10);
            if((0 != errno) || (next1 == next2))
                break;
            useDefaults = false;
            break;
        }
    }

    if(useDefaults)
    {
        masterPath = DefaultVtMaster;
        input = DefaultVtInput;
        output = DefaultVtOutput;
    }
    
    status = ApiOpenFile(masterPath, IO_FILE_READ, IO_FILE_FLAG_SHARED, &master);
    if(OK != status)
        return false;

    status = ApiCreateVt(master, input, output, &name[5]);
    ApiCloseFile(master);
    if(OK != status)
        return false;

    if(OK != PrepareStdHandles(name))
        return false;
    
    return true;
}