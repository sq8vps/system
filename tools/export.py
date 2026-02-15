import exportlib

def exportDriverApi():
    #export block start keyword
    EXPORT_BLOCK_START_KEYWORD = "DRIVER_API"
    #export block end keyword
    EXPORT_BLOCK_END_KEYWORD = "END_DRIVER_API"
    #output files path
    OUTPUT_PATH = "./api/ddk/"
    #remove Nabla API keywords
    DROP_KEYWORDS = ("NABLA_API", "END_NABLA_API")

    #path to search for header files
    KERNEL_SEARCH_PATH = "./kernel/"

    exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_BLOCK_START_KEYWORD, EXPORT_BLOCK_END_KEYWORD, 
                        OUTPUT_PATH, DROP_KEYWORDS = DROP_KEYWORDS, copyIncludes = True)

def exportNablaApi():
    #export block start keyword
    EXPORT_BLOCK_START_KEYWORD = "NABLA_API"
    #export block end keyword
    EXPORT_BLOCK_END_KEYWORD = "END_NABLA_API"
    #output files path
    OUTPUT_PATH = "./api/nabla/"
    #remove Nabla API keywords
    DROP_KEYWORDS = ("DRIVER_API", "END_DRIVER_API")
    #path to search for header files
    KERNEL_SEARCH_PATH = "./kernel/"

    exportlib.exportHeader(KERNEL_SEARCH_PATH, EXPORT_BLOCK_START_KEYWORD, EXPORT_BLOCK_END_KEYWORD, 
                        OUTPUT_PATH, DROP_KEYWORDS = DROP_KEYWORDS, includes = ("defines.h", ))

exportDriverApi()
exportNablaApi()