#ifndef VGA_H_
#define VGA_H_

#include "defines.h"
#include "io/video/config.h"
#include "ke/core/mutex.h"

struct IoRp;

#define VGA_MAX_MODES 128

#define VGA_MAX_DISPLAY_TIMINGS 29

enum VgaInfoType
{
    VGA_INFO_ADAPTER = 1,
    VGA_INFO_DISPLAY = 2,
};

struct VgaTiming
{
    uint32_t clock; /**< Pixel clock */
    struct
    {
        uint16_t x; /**< Horizontal pixels */
        uint16_t y; /**< Vertical pixels (lines) */
    } total, usable, porch, sync; /**< Total pixels, usable pixels, front porch pixels, sync width */
    struct
    {
        uint32_t interlaced : 1; /**< Interlaced */
        uint32_t doubleScan : 1; /**< Double scanned */
        uint32_t hSync : 1; /**< Horizontal sync: 0 - positive, 1 - negative */
        uint32_t vSync : 1; /**< Vertical sync: 0 - positive, 1 - negative */
    } flags; /**< Timing flags */
};

struct VgaDisplayInfo
{
    enum VgaInfoType type; /**< Structure type */
    char id[8]; /**< Display ID - 3 char vendor + 4 char hex device */
    uint32_t serial; /**< Serial number */

    struct VgaTiming timing[VGA_MAX_DISPLAY_TIMINGS]; /**< Reported timings */
    size_t timingCount; /**< Number of reported timings */
    size_t preferredTiming; /**< Preferred timing index in \a timing array */

    struct IoDeviceObject *adapter; /**< Parent video adapter */
};

struct VgaAdapterInfo
{
    enum VgaInfoType type; /**< Structure type */
    struct
    {
        struct IoFrameBufferConfig config; /**< Frame buffer configuration */
        uint16_t mode; /**< VBE mode number */
        PADDRESS lfb; /**< Linear framebuffer address */
        uint32_t maxClock; /**< Maximum pixel clock */
        struct
        {
            uint32_t interlaced : 1; /**< Interlaced mode available? */
            uint32_t doubleScan : 1; /**< Double scanned mode available? */
        } available;
        
    } mode[VGA_MAX_MODES];
    size_t modeCount; /**< Number of available modes */

    uint16_t resetMode; /**< Number of a fundamental VGA mode to be used on adapter reset, e.g, on kernel panic */

    struct IoDeviceObject *display; /**< Display device object */
    KeSpinlock lock; /**< Adapter info lock */
};

/**
 * @brief Enumerate and set up displays
 * @param *rp Request Packet
 */
void VgaEnumerateDisplays(struct IoRp *rp);

/**
 * @brief Handle get device ID request
 * @param *rp Request Packet
 */
void VgaGetDisplayDeviceId(struct IoRp *rp);

/**
 * @brief Find timing data for given mode
 * @param *display Display info structure
 * @param *adapter Adapter info structure
 * @param mode Mode index within the adapter list
 * @return Corresponding timing data or NULL if not available
 */
const struct VgaTiming* VgaFindTimingForMode(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t mode);

/**
 * @brief Find adapter mode for given timing
 * @param *display Display info structure
 * @param *adapter Adapter info structure
 * @param timing Timing index in display timing list
 * @return Index of matching mode in adapter mode list, -1 if not matching mode
 */
size_t VgaFindModeForTiming(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t timing);

/**
 * @brief Check adapter-display mode compatibility by adapter mode number
 * @param *display Display info structure
 * @param *adapter Adapter info structure
 * @param mode Mode index in adapter mode list
 * @return True if compatible, false otherwise
 */
bool VgaCheckModeCompatibility(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t mode);

/**
 * @brief Check adapter-display mode compatibility by display timing mode number
 * @param *display Display info structure
 * @param *adapter Adapter info structure
 * @param timing Timing mode index in display timing list
 * @return True if compatible, false otherwise
 */
bool VgaCheckModeCompatibilityByTiming(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t timing);

/**
 * @brief Find best perfect-matching mode
 * @param *display Display info structure
 * @param *adapter Adapter info structure
 * @return Mode index within the adapter list mode or -1 if none available
 */
size_t VgaFindBestPerfectMatchingMode(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter);

#endif