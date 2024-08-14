import exportlib


#export single keyword/indicator
EXPORT_KEYWORD = "EXPORT"
#export block start keyword
EXPORT_BLOCK_START_KEYWORD = "EXPORT_API"
#export block end keyword
EXPORT_BLOCK_END_KEYWORD = "END_EXPORT_API"
#output files path
OUTPUT_PATH = "./api/"

#path to search for header files
KERNEL_SEARCH_PATH = "./kernel32/"
#output file name
KERNEL_OUTPUT_FILE = "kernel.h"

exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_KEYWORD, EXPORT_BLOCK_START_KEYWORD, EXPORT_BLOCK_END_KEYWORD, 
                       OUTPUT_PATH, KERNEL_OUTPUT_FILE)