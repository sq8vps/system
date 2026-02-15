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
from pathlib import Path

def exportHeader(SEARCH_PATH, EXPORT_START_KEYWORDS, EXPORT_END_KEYWORDS, 
                OUTPUT_PATH, DROP_KEYWORDS = None, copyIncludes = False, includes = None):
    print("Exporting to " + OUTPUT_PATH)
    print("Headers from " + SEARCH_PATH + " (recursively)")
    print('\tBlock export keywords are "' + str(EXPORT_START_KEYWORDS) + '" and "' + str(EXPORT_END_KEYWORDS) + '"')
    #is .def instead of .h?
    isDef = False
    #counters for statistics
    exports = 0
    #look recursively for header files
    for headerPath in (glob.glob(SEARCH_PATH  + "**/*.h", recursive = True) + glob.glob(SEARCH_PATH  + "**/*.def", recursive = True)):
        if Path(headerPath).suffix == ".def":
            isDef = True
        exportsInFile = 0
        newHeaderPath = OUTPUT_PATH + '/' + headerPath.replace(SEARCH_PATH, '')
        os.makedirs(newHeaderPath.rsplit('/', 1)[0], exist_ok = True)
        with open(newHeaderPath, "w") as newHeader:
            newHeader.write(COMMENT)
            if isDef == False:
                newHeader.write("#ifndef EXPORTED_" + newHeaderPath.upper().replace('/', '_').replace('.', '_').replace('-', '_') + "_" + "\n")
                newHeader.write("#define EXPORTED_" + newHeaderPath.upper().replace('/', '_').replace('.', '_') + "_" + "\n")
                newHeader.write(FILE_PROLOGUE)
            with open(headerPath, "r") as header:
                content = header.readlines()
                keywordFound = False

                if includes != None:
                    for inc in includes:
                        newHeader.write("#include \"{}\"\n".format(inc))

                for line in content:
                    if line.startswith("#include") and copyIncludes:
                        newHeader.write(line.replace(SEARCH_PATH, ''))
                        continue

                    if line.startswith(EXPORT_START_KEYWORDS):
                        keywordFound = True
                        exports += 1
                        exportsInFile += 1
                        for k in EXPORT_START_KEYWORDS:
                            line = line.replace(k, "")
                        if not line.strip(): #is line empty after removing the export keyword?
                            continue

                    if line.startswith(EXPORT_END_KEYWORDS):
                        keywordFound = False
                        for k in EXPORT_END_KEYWORDS:
                            line = line.replace(k, "")
                        if not line.strip():
                            continue

                    if DROP_KEYWORDS != None:
                        for keyword in DROP_KEYWORDS:
                            if line.startswith(keyword):
                                line = line[len(keyword):]
                                break
                        if not line.strip():
                            continue

                        
                    if keywordFound:
                        newHeader.write(line)
            if isDef == False:
                newHeader.write(FILE_EPILOGUE)
                newHeader.write("#endif")
        if exportsInFile == 0:
            os.remove(newHeaderPath)
    print("\tExported " + str(exports) + " blocks")



