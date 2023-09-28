//This header file is generated automatically
#ifndef EXPORTED___API__IT_IT_H_
#define EXPORTED___API__IT_IT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
/**
 * @brief ISR frame for interrupts and exceptions without an error code with privilege level change
*/
struct ItFrameMS
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} PACKED;

/**
 * @brief Get free interrupt vector number
 * @return Free vector number or 0 on failure
*/
extern uint8_t ItGetFreeVector(void);

/**
 * @brief Install interrupt handler
 * @param vector Interrupt vector number
 * @param *isr Interrupt service routine pointer
 * @param *context Context to be passed to ISR
 * @param cpl Required privilege level to use this interrupt
 * @return Status code
*/
extern STATUS ItInstallInterruptHandler(uint8_t vector, void (*isr)(void*), void *context, PrivilegeLevel_t cpl);

/**
 * @brief Uninstall interrupt handler
 * @param vector Vector number
 * @return Status code
*/
extern STATUS ItUninstallInterruptHandler(uint8_t vector);


#ifdef __cplusplus
}
#endif

#endif