#include "device.h"
#include "hal/i686/ioport.h"
#include "hal/i686/irq.h"
#include "hal/interrupt.h"
#include "ps2.h"

struct I8042Controller I8042ControllerInfo = {.lock = KeSpinlockInitializer};

#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_COMMAND_PORT 0x64

#define I8042_FIRST_ISA_IRQ 1
#define I8042_SECOND_ISA_IRQ 12

#define I8042_STATUS_OUTPUT_BUFFER_FULL_BIT 0x1
#define I8042_STATUS_INPUT_BUFFER_FULL_BIT 0x2
#define I8042_STATUS_TIME_OUT_ERROR_BIT 0x40
#define I8042_STATUS_PARITY_ERROR_BIT 0x80

#define I8042_COMMAND_READ_CONFIG 0x20
#define I8042_COMMAND_WRITE_CONFIG 0x60
#define I8042_COMMAND_DISABLE_FIRST_PORT 0xAD
#define I8042_COMMAND_ENABLE_FIRST_PORT 0xAE
#define I8042_COMMAND_DISABLE_SECOND_PORT 0xA7
#define I8042_COMMAND_ENABLE_SECOND_PORT 0xA8
#define I8042_COMMAND_TEST_SECOND_PORT 0xA9
#define I8042_COMMAND_TEST_CONTROLLER 0xAA
#define I8042_COMMAND_TEST_FIRST_PORT 0xAB
#define I8042_COMMAND_SELECT_SECOND_PORT 0xD4

#define I8042_PORT_TEST_SUCCESS 0x00
#define I8042_CONTROLLER_TEST_SUCCESS 0x55

#define I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT 0x1
#define I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT 0x2
#define I8042_CONFIG_FIRST_PORT_CLOCK_DISABLE_BIT 0x10
#define I8042_CONFIG_SECOND_PORT_CLOCK_DISABLE_BIT 0x20
#define I8042_CONFIG_FIRST_PORT_TRANSLATION_ENABLE_BIT 0x40

#define I8042_TIMEOUT 100

static inline bool I8042_WAIT_FOR_READY_WRITE(uint32_t timeout)
{
    while(IoPortReadByte(I8042_STATUS_PORT) & I8042_STATUS_INPUT_BUFFER_FULL_BIT)
    {
        if(0 == --timeout)
            return false;
    }
    return true;
} 
static inline bool I8042_WAIT_FOR_READY_READ(uint32_t timeout)
{
    while(!(IoPortReadByte(I8042_STATUS_PORT) & I8042_STATUS_OUTPUT_BUFFER_FULL_BIT))
    {
        if(0 == --timeout)
            return false;
    }
    return true;
} 

static void I8042WriteToController(uint8_t command, uint8_t data)
{
    I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, command);
    I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
    IoPortWriteByte(I8042_DATA_PORT, data);
}

static uint8_t I8042ReadFromController(uint8_t command)
{
    I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, command);
    I8042_WAIT_FOR_READY_READ(I8042_TIMEOUT);
    return IoPortReadByte(I8042_DATA_PORT);
}

void I8042SetInterruptsEnabled(uint8_t port, bool state)
{
    uint8_t config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
    if(state)
        config |= ((PORT_SECOND == port) ? I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT : I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT);
    else
        config &= ~((PORT_SECOND == port) ? I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT : I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT);
    I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config);
}

STATUS I8402HandleIrq(void *context)
{
    Ps2HandleIrq((uintptr_t)context, IoPortReadByte(I8042_DATA_PORT));
    return OK;
}

STATUS I8042InitializeController(void)
{
    STATUS status = OK;
    //disable ports
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_FIRST_PORT);
    I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_SECOND_PORT);
    //flush buffer
    IoPortReadByte(I8042_DATA_PORT);
    //disable port[PORT_FIRST] port IRQ and translation and enable port[PORT_FIRST] port clock
    uint8_t config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
    config &= ~(I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT | I8042_CONFIG_FIRST_PORT_TRANSLATION_ENABLE_BIT
        | I8042_CONFIG_FIRST_PORT_CLOCK_DISABLE_BIT);
    I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config); 
    //perform controller test
    if(I8042_CONTROLLER_TEST_SUCCESS != I8042ReadFromController(I8042_COMMAND_TEST_CONTROLLER))
    {
        return DEVICE_NOT_AVAILABLE;
    }
    //try to enable port[PORT_SECOND] port to check if it exists
    I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_SECOND_PORT);
    config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
    if(config & I8042_CONFIG_SECOND_PORT_CLOCK_DISABLE_BIT)
        I8042ControllerInfo.port[PORT_SECOND].usable = 0;
    else
    {
        I8042ControllerInfo.port[PORT_SECOND].usable = 1;
        I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_SECOND_PORT);
        config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
        config &= ~(I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT | I8042_CONFIG_SECOND_PORT_CLOCK_DISABLE_BIT);
        I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config);
    }

    if(I8042_PORT_TEST_SUCCESS == I8042ReadFromController(I8042_COMMAND_TEST_FIRST_PORT))
        I8042ControllerInfo.port[PORT_FIRST].usable = 1;
    if(I8042ControllerInfo.port[PORT_SECOND].usable && (I8042_PORT_TEST_SUCCESS == I8042ReadFromController(I8042_COMMAND_TEST_SECOND_PORT)))
        I8042ControllerInfo.port[PORT_SECOND].usable = 1;
    else
        I8042ControllerInfo.port[PORT_SECOND].usable = 0;
    
    if(I8042ControllerInfo.port[PORT_FIRST].usable)
    {
        I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_FIRST_PORT);

        I8042ControllerInfo.port[PORT_FIRST].gsi = I686ResolveIsaIrqMapping(I8042_FIRST_ISA_IRQ);
        status = HalRegisterIrq(I8042ControllerInfo.port[PORT_FIRST].gsi, I8402HandleIrq, (void*)PORT_FIRST, 
            (struct HalInterruptParams){.mode = HAL_IT_MODE_FIXED, .polarity = HAL_IT_POLARITY_ACTIVE_HIGH, 
            .trigger = HAL_IT_TRIGGER_EDGE, .wake = HAL_IT_WAKE_INCAPABLE, .shared = HAL_IT_NOT_SHAREABLE});
        if(OK != status)
            return status;
        status = HalEnableIrq(I8042ControllerInfo.port[PORT_FIRST].gsi, I8402HandleIrq);
        if(OK != status)
            return status;
    }
    if(I8042ControllerInfo.port[PORT_SECOND].usable)
    {
        I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_SECOND_PORT);

        I8042ControllerInfo.port[PORT_SECOND].gsi = I686ResolveIsaIrqMapping(I8042_SECOND_ISA_IRQ);
        status = HalRegisterIrq(I8042ControllerInfo.port[PORT_SECOND].gsi, I8402HandleIrq, (void*)PORT_SECOND, 
            (struct HalInterruptParams){.mode = HAL_IT_MODE_FIXED, .polarity = HAL_IT_POLARITY_ACTIVE_HIGH, 
            .trigger = HAL_IT_TRIGGER_EDGE, .wake = HAL_IT_WAKE_INCAPABLE, .shared = HAL_IT_NOT_SHAREABLE});
        if(OK != status)
            return status;
        status = HalEnableIrq(I8042ControllerInfo.port[PORT_SECOND].gsi, I8402HandleIrq);
        if(OK != status)
            return status;
    }
    return OK;
}

bool I8042WriteToPeripheral(uint8_t port, uint8_t data)
{
    bool status = false;

    if(PORT_SECOND == port)
    {
        if(likely(I8042ControllerInfo.port[PORT_SECOND].usable))
        {
            if(I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT))
            {
                IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_SELECT_SECOND_PORT);
                if(I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT))
                {
                    IoPortWriteByte(I8042_DATA_PORT, data);
                    status = true;
                }
            }
        }
    }
    else
    {
        if(likely(I8042ControllerInfo.port[PORT_FIRST].usable))
        {
            if(I8042_WAIT_FOR_READY_WRITE(I8042_TIMEOUT))
            {
                IoPortWriteByte(I8042_DATA_PORT, data);
                status = true;
            }
        }
    }
    return status;
}

bool I8042ReadFromPeripheral(uint8_t *data, uint16_t count)
{
    bool status = false;

    for(uint16_t i = 0; i < count; i++)
    {
        if(I8042_WAIT_FOR_READY_READ(I8042_TIMEOUT))
        {
            data[i] = IoPortReadByte(I8042_DATA_PORT);
            status = true;
        }
        else
        {
            status = false;
            break;
        }
    }
    return status;
}
