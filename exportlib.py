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

def exportHeader(SEARCH_PATH, EXPORT_KEYWORD, EXTERN_KEYWORD, EXTERN_KEYWORD_REPLACEMENT, 
                OUTPUT_PATH, OUTPUT_FILE):
    print("Generating " + OUTPUT_PATH + OUTPUT_FILE)
    print("Headers from " + SEARCH_PATH + " (recursively)")
    print('\tExport keyword is "' + EXPORT_KEYWORD + '"')
    print('\tExtern keyword is "' + EXTERN_KEYWORD + '", replacing with "' + EXTERN_KEYWORD_REPLACEMENT + '"')
    #open main file
    with open(OUTPUT_PATH + OUTPUT_FILE, "w") as outputFile:
        #write head for main file
        outputFile.write(COMMENT)
        outputFile.write("#ifndef EXPORTED_" + OUTPUT_FILE.upper().replace('.', '_').replace('-', '_') + "_" + "\n")
        outputFile.write("#define EXPORTED_" + OUTPUT_FILE.upper().replace('.', '_') + "_" + "\n")
        outputFile.write(FILE_PROLOGUE)
        #counters for statistics
        exports = 0
        externs = 0
        #look recursively for header files
        for headerPath in glob.glob(SEARCH_PATH  + "**/*.h", recursive = True):
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
                    brackets = 0 #+1 for one opening bracket, -1 for one closing bracket
                    for line in content:
                        if line.startswith("#include"):
                            newHeader.write(line.replace(SEARCH_PATH, ''))
                            continue

                        if line.startswith(EXPORT_KEYWORD):
                            keywordFound = True
                            exports += 1
                            line = line.replace(EXPORT_KEYWORD, "")
                            if not line.strip(): #is line empty after removing the export keyword?
                                continue

                        if keywordFound:
                            if "{" in line:
                                brackets += 1
                            if "}" in line:
                                brackets -= 1

                            if (not line.strip()) and (not brackets): #got empty line
                                newHeader.write("\n")
                                keywordFound = False
                            else:
                                if EXTERN_KEYWORD in line:
                                    externs += 1
                                newHeader.write(line.replace(EXTERN_KEYWORD, EXTERN_KEYWORD_REPLACEMENT))
                newHeader.write(FILE_EPILOGUE)
                newHeader.write("#endif")
        print("\tFound " + str(exports) + " occurences of " + EXPORT_KEYWORD + " and " + str(externs) + " occurences of " + EXTERN_KEYWORD)
        outputFile.write(FILE_EPILOGUE)
        outputFile.write("#endif")



