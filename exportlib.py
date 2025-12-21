# A small script for automatic preparation of kernel API C header

COMMENT = "//This header file is generated automatically\n"
FILE_PROLOGUE = """
#ifdef __cplusplus
extern "C" 
{
#endif

"""
FILE_EPILOGUE = """
#ifdef __cplusplus
}
#endif

"""

import glob
import os

def exportHeader(SEARCH_PATH, EXPORT_START_KEYWORD, EXPORT_END_KEYWORD, 
                OUTPUT_PATH, skipIncludes = False):
    print("Exporting to " + OUTPUT_PATH)
    print("Headers from " + SEARCH_PATH + " (recursively)")
    print('\tBlock export keywords are "' + EXPORT_START_KEYWORD + '" and "' + EXPORT_END_KEYWORD + '"')
    #counters for statistics
    exports = 0
    #look recursively for header files
    for headerPath in glob.glob(SEARCH_PATH  + "**/*.h", recursive = True):
        exportsInFile = 0
        newHeaderPath = OUTPUT_PATH + '/' + headerPath.replace(SEARCH_PATH, '')
        os.makedirs(newHeaderPath.rsplit('/', 1)[0], exist_ok = True)
        with open(newHeaderPath, "w") as newHeader:
            newHeader.write(COMMENT)
            newHeader.write("#ifndef EXPORTED_" + newHeaderPath.upper().replace('/', '_').replace('.', '_').replace('-', '_') + "_" + "\n")
            newHeader.write("#define EXPORTED_" + newHeaderPath.upper().replace('/', '_').replace('.', '_') + "_" + "\n")
            newHeader.write(FILE_PROLOGUE)
            with open(headerPath, "r") as header:
                content = header.readlines()
                keywordFound = False
                for line in content:
                    if line.startswith("#include") and not skipIncludes:
                        newHeader.write(line.replace(SEARCH_PATH, ''))
                        continue

                    if line.startswith(EXPORT_START_KEYWORD):
                        keywordFound = True
                        exports += 1
                        exportsInFile += 1
                        line = line.replace(EXPORT_START_KEYWORD, "")
                        if not line.strip(): #is line empty after removing the export keyword?
                            continue

                    if line.startswith(EXPORT_END_KEYWORD):
                        keywordFound = False
                        line = line.replace(EXPORT_END_KEYWORD, "")
                        if not line.strip():
                            continue
                        
                    if keywordFound:
                        newHeader.write(line)

            newHeader.write(FILE_EPILOGUE)
            newHeader.write("#endif")
        if exportsInFile == 0:
            os.remove(newHeaderPath)
    print("\tExported " + str(exports) + " blocks")



