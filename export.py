import exportlib


#export keyword/indicator
EXPORT_KEYWORD = "EXPORT"
#extern keyword
EXTERN_KEYWORD = "EXTERN"
#how to replace extern keyword
EXTERN_KEYWORD_REPLACEMENT = "extern"
#output files path
OUTPUT_PATH = "./api/"

#path to search for header files
KERNEL_SEARCH_PATH = "./kernel32/"
#output file name
KERNEL_OUTPUT_FILE = "kernel.h"

exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_KEYWORD, EXTERN_KEYWORD, EXTERN_KEYWORD_REPLACEMENT, 
                       OUTPUT_PATH, KERNEL_OUTPUT_FILE)