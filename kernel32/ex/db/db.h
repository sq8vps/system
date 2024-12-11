#ifndef EX_DB_H_
#define EX_DB_H_

#include <stdint.h>
#include "defines.h"

struct IoFileHandle;
struct NablaDbHeader;
struct NablaDbEntry;

/**
 * @brief Database handle structure
 */
struct ExDbHandle
{
    int file;
    uint64_t size;
    struct NablaDbHeader *db;
    struct NablaDbEntry *last;
    struct NablaDbEntry *array;
};

/**
 * @brief Open system database file
 * @param *path Database path
 * @param **h Output database handle
 * @return Status code
 */
STATUS ExDbOpen(const char *path, struct ExDbHandle **h);

/**
 * @brief Close system database file
 * @param *h Database handle
 */
void ExDbClose(struct ExDbHandle *h);

/**
 * @brief Get (next) string with given name from database
 * @param *h Database handle
 * @param *name Entry name
 * @param **str Output string pointer
 * @return Status code. If different than \a OK, then \a str is invalid.
 * @note This function can be called multiple times, e. g., to obtain multiple entries from an array.
 * If the end of array or database is reached, this function fails, the internal state is cleared
 * and at the next call the search starts from the beginning.
 * Use \a ExDbRewind() to clear internal state manually.
 */
STATUS ExDbGetNextString(struct ExDbHandle *h, const char *name, char **str);

/**
 * @brief Get (next) boolean with given name from database
 * @param *h Database handle
 * @param *name Entry name
 * @param *b Output boolean
 * @return Status code. If different than \a OK, then \a b is invalid.
 * @note This function can be called multiple times, e. g., to obtain multiple entries from an array.
 * If the end of array or database is reached, this function fails, the internal state is cleared
 * and at the next call the search starts from the beginning.
 * Use \a ExDbRewind() to clear internal state manually.
 */
STATUS ExDbGetNextBool(struct ExDbHandle *h, const char *name, bool *b);

/**
 * @brief Rewind database search
 * @param *h Database handle
 */
void ExDbRewind(struct ExDbHandle *h);

#endif