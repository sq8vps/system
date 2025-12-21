import exportlib
import shutil

def exportSyscall():
    #export block start keyword
    EXPORT_BLOCK_START_KEYWORD = "EXPORT_SYSCALL"
    #export block end keyword
    EXPORT_BLOCK_END_KEYWORD = "END_EXPORT_SYSCALL"
    #output files path
    OUTPUT_PATH = "./sys/"

    #path to search for header files
    KERNEL_SEARCH_PATH = "./kernel32/ke/sys/"

    exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_BLOCK_START_KEYWORD, EXPORT_BLOCK_END_KEYWORD, 
                        OUTPUT_PATH, skipIncludes = True)
    
    shutil.copyfile("./kernel32/status.h", OUTPUT_PATH + "status.h")