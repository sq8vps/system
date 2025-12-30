//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_TASKFS_H_
#define EXPORTED___API__IO_FS_TASKFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include "ke/task/task.h"

/**
 * @addtogroup io_fs_taskfs Task file system (\c /task) support
 * @brief Device file system (\c /task) support
 * @kinternal
 * @ingroup io_fs
 * @{
 */

/**
 * @brief Task file system context used for accessing /taskfs
 */
struct IoTaskFsContext
{
    KE_TASK_ID tid; 
    int fd;
};

/**
 * @brief Macro used to initialize \ref IoTaskFsContext
 */
#define IO_TASK_FS_CONTEXT_INITIALIZER {.tid = -1, .fd = -1}


#ifdef __cplusplus
}
#endif

#endif