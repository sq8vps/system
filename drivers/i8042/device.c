#include "device.h"
#include "hal/i686/ioport.h"

struct I8042Controller I8042ControllerInfo = {.lock = KeSpinlockInitializer};

#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_COMMAND_PORT 0x64

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

#define I8042_TIMEOUT 100000

static inline bool I8042_WAIT_FOR_WRITE_READY(uint32_t timeout)
{
    while(IoPortReadByte(I8042_STATUS_PORT) & I8042_STATUS_INPUT_BUFFER_FULL_BIT)
    {
        if(0 == --timeout)
            return false;
    }
    return true;
} 
static inline bool I8042_WAIT_FOR_READ_READY(uint32_t timeout)
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
    I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, command);
    I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
    IoPortWriteByte(I8042_DATA_PORT, data);
}

static uint8_t I8042ReadFromController(uint8_t command)
{
    I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, command);
    I8042_WAIT_FOR_READ_READY(I8042_TIMEOUT);
    return IoPortReadByte(I8042_DATA_PORT);
}

STATUS I8042InitializeController(void)
{
    //disable ports
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_FIRST_PORT);
    I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_SECOND_PORT);
    //flush buffer
    IoPortReadByte(I8042_DATA_PORT);
    //disable first port IRQ and translation and enable first port clock
    uint8_t config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
    config &= ~(I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT | I8042_CONFIG_FIRST_PORT_TRANSLATION_ENABLE_BIT
        | I8042_CONFIG_FIRST_PORT_CLOCK_DISABLE_BIT);
    I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config); 
    //perform controller test
    if(I8042_CONTROLLER_TEST_SUCCESS != I8042ReadFromController(I8042_COMMAND_TEST_CONTROLLER))
    {
        return DEVICE_NOT_AVAILABLE;
    }
    //try to enable second port to check if it exists
    I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
    IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_SECOND_PORT);
    config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
    if(config & I8042_CONFIG_SECOND_PORT_CLOCK_DISABLE_BIT)
        I8042ControllerInfo.usable.second = 0;
    else
    {
        I8042ControllerInfo.usable.second = 1;
        I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_DISABLE_SECOND_PORT);
        config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
        config &= ~(I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT | I8042_CONFIG_SECOND_PORT_CLOCK_DISABLE_BIT);
        I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config);
    }

    if(I8042_PORT_TEST_SUCCESS == I8042ReadFromController(I8042_COMMAND_TEST_FIRST_PORT))
        I8042ControllerInfo.usable.first = 1;
    if(I8042ControllerInfo.usable.second && (I8042_PORT_TEST_SUCCESS == I8042ReadFromController(I8042_COMMAND_TEST_SECOND_PORT)))
        I8042ControllerInfo.usable.second = 1;
    else
        I8042ControllerInfo.usable.second = 0;
    
    if(I8042ControllerInfo.usable.first)
    {
        I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_FIRST_PORT);
        // config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
        // config |= I8042_CONFIG_FIRST_PORT_IRQ_ENABLE_BIT;
        // I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config);
    }
    if(I8042ControllerInfo.usable.second)
    {
        I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT);
        IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_ENABLE_SECOND_PORT);
        // config = I8042ReadFromController(I8042_COMMAND_READ_CONFIG);
        // config |= I8042_CONFIG_SECOND_PORT_IRQ_ENABLE_BIT;
        // I8042WriteToController(I8042_COMMAND_WRITE_CONFIG, config);
    }
    return OK;
}

bool I8042WriteToPeripheral(bool device, uint8_t data)
{
    bool status = false;

    if(device)
    {
        if(likely(I8042ControllerInfo.usable.second))
        {
            if(I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT))
            {
                IoPortWriteByte(I8042_COMMAND_PORT, I8042_COMMAND_SELECT_SECOND_PORT);
                if(I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT))
                {
                    IoPortWriteByte(I8042_DATA_PORT, data);
                    status = true;
                }
            }
        }
    }
    else
    {
        if(likely(I8042ControllerInfo.usable.first))
        {
            if(I8042_WAIT_FOR_WRITE_READY(I8042_TIMEOUT))
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
        if(I8042_WAIT_FOR_READ_READY(I8042_TIMEOUT))
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

