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

def exportHeader(SEARCH_PATH, EXPORT_KEYWORD, EXPORT_START_KEYWORD, EXPORT_END_KEYWORD, 
                OUTPUT_PATH, OUTPUT_FILE):
    print("Generating " + OUTPUT_PATH + OUTPUT_FILE)
    print("Headers from " + SEARCH_PATH + " (recursively)")
    print('\tBlock export keywords are "' + EXPORT_START_KEYWORD + '" and "' + EXPORT_END_KEYWORD + '"')
    #open main file
    with open(OUTPUT_PATH + OUTPUT_FILE, "w") as outputFile:
        #write head for main file
        outputFile.write(COMMENT)
        outputFile.write("#ifndef EXPORTED_" + OUTPUT_FILE.upper().replace('.', '_').replace('-', '_') + "_" + "\n")
        outputFile.write("#define EXPORTED_" + OUTPUT_FILE.upper().replace('.', '_') + "_" + "\n")
        outputFile.write(FILE_PROLOGUE)
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
                    outputFile.write('#include "{}"'.format(headerPath.replace(SEARCH_PATH, '')) + '\n')
                    content = header.readlines()
                    keywordFound = False
                    for line in content:
                        if line.startswith("#include"):
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
        outputFile.write(FILE_EPILOGUE)
        outputFile.write("#endif")



