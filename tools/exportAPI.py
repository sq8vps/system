import exportlib

def exportAPI():
    #export block start keyword
    EXPORT_BLOCK_START_KEYWORD = "EXPORT_API"
    #export block end keyword
    EXPORT_BLOCK_END_KEYWORD = "END_EXPORT_API"
    #output files path
    OUTPUT_PATH = "./api/"

    #path to search for header files
    KERNEL_SEARCH_PATH = "./kernel/"

    exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_BLOCK_START_KEYWORD, EXPORT_BLOCK_END_KEYWORD, 
                        OUTPUT_PATH)