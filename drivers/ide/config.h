#ifndef IDE_CONFIG_H_
#define IDE_CONFIG_H_

#include "defines.h"
#include <stdint.h>
#include "device.h"

/**
 * @brief Clear Physical Region Descriptor table
 * @param *t PRD table pointer
 * @return Status code
*/
STATUS IdeClearPrdTable(struct IdePrdTable *t);

/**
 * @brief Initialize Physical Region Descriptor tables - allocate memory and clear
 * @param *info Controller structure pointer
 * @return Status code
 * @attention This function should be called only once
*/
STATUS IdeInitializePrdTables(struct IdeControllerData *info);

/**
 * @brief Insert PRD entry to given table
 * @param *table PRD table pointer
 * @param address Physical region address
 * @param size Physical region size
*/
uint32_t IdeAddPrdEntry(struct IdePrdTable *table, uint32_t address, uint16_t size);

/**
 * @brief Perform full IDE controller initialization
 * @param *bdo Base Device Object
 * @param *mdo Main Device Object of this controller
 * @param *info Controller structure pointer
 * @return Status code
 * @attention This function should be called only once
*/
STATUS IdeConfigureController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct IdeControllerData *info);

/**
 * @brief Issue a Bus Master command
 * @param *info Controller data structure
 * @param chan Channel number
 * @param command Command to issue
*/
void IdeWriteBmrCommand(struct IdeControllerData *info, uint8_t chan, uint8_t command);

/**
 * @brief Read Bus Master status register
 * @param *info Controller data structure
 * @param chan Channel number
 * @return Status register value
*/
uint8_t IdeReadBmrStatus(struct IdeControllerData *info, uint8_t chan);

/**
 * @brief Write Bus Master status register
 * @param *info Controller data structure
 * @param chan Channel number
 * @param status Status to write
*/
void IdeWriteBmrStatus(struct IdeControllerData *info, uint8_t chan, uint8_t status);

/**
 * @brief Write Bus Master Physical Region Descriptor Table Register
 * @param *info Controller data structure
 * @param chan Channel number
 * @param *prdt PRD Table structure pointer
*/
void IdeWriteBmrPrdt(struct IdeControllerData *info, uint8_t chan, struct IdePrdTable *prdt);

#endif