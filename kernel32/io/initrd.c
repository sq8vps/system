#include "initrd.h"
#include "multiboot.h"
#include "common.h"
#include "mm/dynmap.h"
#include "io/fs/vfs.h"
#include "assert.h"

#define MB2_INITRD_STRING "initrd"

/* 
    GNU tar Archive Format description.

   Copyright 1988-2024 Free Software Foundation, Inc.

   This file is part of GNU tar.

   GNU tar is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GNU tar is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
*/

struct TarHeader
{				
  uint8_t name[100];
  uint8_t mode[8];	
  uint8_t uid[8];
  uint8_t gid[8];
  uint8_t size[12];
  uint8_t mtime[12];
  uint8_t chksum[8];
  uint8_t typeflag;
  uint8_t linkname[100];
  uint8_t magic[6];
  uint8_t version[2];
  uint8_t uname[32];
  uint8_t gname[32];
  uint8_t devmajor[8];
  uint8_t devminor[8];
  uint8_t prefix[155];

  uint8_t reserved[12]; //fill to 512 bytes
} PACKED;

#define TAR_GNU_MAGIC_VERSION   "ustar  \0"
#define TAR_POSIX_MAGIC_VERSION "ustar\0" "00"

enum TarType
{
    TAR_REGULAR	= '0', //regular file
    TAR_AREGULAR = '\0', //alternative regular file
    TAR_LINK = '1', //symbolic link
    TAR_DIRECTORY = '5', //directory
};

#define TAR_RECURSION_DEPTH_LIMIT 8

static struct TarHeader *IoInitrdHandle = NULL;

static bool IoInitrdVerifyChecksum(const struct TarHeader *h)
{
    uint32_t sum = 0;
    const uint8_t *t = (const uint8_t*)h;
    //checksum must be calculated with checksum field set to spaces
    //calculate a sum of everything first
    for(uint32_t i = 0; i < 500; i++)
    {
        sum += t[i];
    }
    //remove sum of checksum field bytes
    for(uint32_t i = 0; i < sizeof(h->chksum); i++)
    {
        sum -= h->chksum[i];
    }
    //sum up spaces which should replace the checksum
    sum += (sizeof(h->chksum) * ' ');
    //verify checksum
    if(CmOctalToU32((const char*)h->chksum) == sum)
        return true;
    else
        return false;
}

static struct TarHeader* IoInitrdFindFile(const char *path)
{
    struct TarHeader *h = IoInitrdHandle;
    while(1)
    {
        if((0 == CmMemcmp(h->magic, TAR_GNU_MAGIC_VERSION, sizeof(TAR_GNU_MAGIC_VERSION - 1)))
            || (0 == CmMemcmp(h->magic, TAR_POSIX_MAGIC_VERSION, sizeof(TAR_POSIX_MAGIC_VERSION - 1))))
        {
            if(IoInitrdVerifyChecksum(h))
            {
                uint32_t pathLength = CmStrlen(path);
                if((0 == CmStrncmp(path, (char*)h->name, sizeof(h->name) - 1))
                    || ((TAR_DIRECTORY == h->typeflag) && (0 == CmStrncmp(path, (char*)h->name, pathLength)) 
                            && (h->name[pathLength] == '/')))
                {
                    return h;
                }
                uint32_t blocks = ALIGN_UP(CmOctalToU32((const char*)h->size) + sizeof(*h), 512) / 512;
                h += blocks;
            }
            else
                return NULL;
        }
        else
        {
            //probably end of archive
            return NULL;
        }
    }
}

STATUS IoInitrdInit(const struct Multiboot2InfoHeader *mb2h)
{
    const struct Multiboot2InfoTag *tag = Multiboot2FindTag(mb2h, NULL, MB2_MODULE);
    while(NULL != tag)
    {
        const struct Multiboot2ModuleTag *mod = (const struct Multiboot2ModuleTag*)tag;
        if(0 == CmStrcmp(mod->str, MB2_INITRD_STRING))
        {
            IoInitrdHandle = MmMapDynamicMemory(mod->mod_start, mod->mod_end - mod->mod_start, MM_FLAG_READ_ONLY);
            if(NULL == IoInitrdHandle)
                return OUT_OF_RESOURCES;
            
            return OK;
        }
        tag = Multiboot2FindTag(mb2h, tag, MB2_MODULE);
    } 
    
    return IO_FILE_NOT_FOUND;
}

STATUS IoInitrdMount(char *mountPoint)
{
    ASSERT(mountPoint);

    //sanity check
    if(!CmCheckPath(mountPoint))
        return IO_ILLEGAL_NAME;

    if(IoVfsCheckIfNodeExists(mountPoint))
        return IO_FILE_ALREADY_EXISTS;
    
    struct IoVfsNode *node = NULL;
    node = IoVfsCreateNode(CmGetFileName(mountPoint));
    if(NULL == node)
    {
        return OUT_OF_RESOURCES;
    }

    node->type = IO_VFS_MOUNT_POINT;
    node->fsType = IO_VFS_FS_INITRD;

    return IoVfsInsertNodeByFilePath(node, mountPoint);
}

STATUS IoInitrdGetNode(struct IoVfsNode *parent, const char *name, struct IoVfsNode **node)
{
    uint32_t pathSize = 1;

    const struct IoVfsNode *p = parent;
    while(IO_VFS_MOUNT_POINT != p->type)
    {
        pathSize += CmStrlen(p->name) + 1;
        p = p->parent;
    }
    pathSize += CmStrlen(name);
    char *const path = MmAllocateKernelHeap(pathSize);
    if(NULL == path)
        return OUT_OF_RESOURCES;
    
    CmStrcpy(&path[pathSize - CmStrlen(name) - 1], name);
    pathSize -= CmStrlen(name);

    p = parent;
    while(IO_VFS_MOUNT_POINT != p->type)
    {
        CmStrcpy(&path[pathSize - CmStrlen(p->name) - 2], p->name);
        path[pathSize - 2] = '/';
        pathSize -= (CmStrlen(p->name) + 1);
        p = p->parent;
    }

    struct TarHeader *h = IoInitrdFindFile(path);
    MmFreeKernelHeap(path);
    if(NULL == h)
        return IO_FILE_NOT_FOUND;
    
    *node = IoVfsCreateNode(name);
    if(NULL == *node)
        return OUT_OF_RESOURCES;
    
    switch(h->typeflag)
    {
        case TAR_REGULAR:
        case TAR_AREGULAR:
            (*node)->type = IO_VFS_FILE;
            break;
        case TAR_DIRECTORY:
            (*node)->type = IO_VFS_DIRECTORY;
            break;
        case TAR_LINK:
            (*node)->type = IO_VFS_LINK;
            break;
        default:
            (*node)->type = IO_VFS_UNKNOWN;
            break;
    }
    (*node)->ref[0].p = h;
    (*node)->flags = IO_VFS_FLAG_NO_CACHE | IO_VFS_FLAG_READ_ONLY;
    (*node)->fsType = IO_VFS_FS_INITRD;
    (*node)->size = CmOctalToU32((char*)h->size);
    return OK;
}

uintptr_t IoInitrdRead(const struct IoVfsNode *node, void *buffer, uintptr_t size, uintptr_t offset)
{
    const struct TarHeader *h = node->ref[0].p;
    if(IoInitrdVerifyChecksum(h))
    {
        if(TAR_DIRECTORY == h->typeflag)
            return 0;
        uint32_t fsize = CmOctalToU32((char*)h->size);
        if(offset > fsize)
            return 0;
        if((offset + size) > fsize)
            size = fsize - offset;
        
        CmMemcpy(buffer, (void*)((uintptr_t)(h + 1) + offset), size);
        return size;
    }
    return 0;
}