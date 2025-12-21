#include "uart.h"
#include "ioport.h"
#include "hal/time.h"

#define UART_LINE_CONTROL_DLAB 0x80
#define UART_LINE_CONTROL_5_BITS 0x00
#define UART_LINE_CONTROL_6_BITS 0x01
#define UART_LINE_CONTROL_7_BITS 0x02
#define UART_LINE_CONTROL_8_BITS 0x03
#define UART_LINE_CONTROL_1_STOP_BIT 0x00
#define UART_LINE_CONTROL_2_STOP_BITS 0x04
#define UART_LINE_CONTROL_NO_PARITY 0x00
#define UART_LINE_CONTROL_ODD_PARITY 0x08
#define UART_LINE_CONTROL_EVEN_PARITY 0x18
#define UART_LINE_CONTROL_MARK_PARITY 0x28
#define UART_LINE_CONTROL_SPACE_PARITY 0x38

#define UART_FIFO_ENABLE 0x01
#define UART_FIFO_CLEAR_RECEIVE 0x02
#define UART_FIFO_CLEAR_TRANSMIT 0x04
#define UART_FIFO_THRESHOLD_1_BYTE 0x00
#define UART_FIFO_THRESHOLD_4_BYTES 0x40
#define UART_FIFO_THRESHOLD_8_BYTES 0x80
#define UART_FIFO_THRESHOLD_14_BYTES 0xC0

#define UART_LINE_STATUS_DATA_READY 0x01
#define UART_LINE_STATUS_THR_EMPTY 0x20

#define UART_MODEM_CONTROL_LOOPBACK 0x10

/* UART configuration */
#define UART_BAUDRATE 115200
#define UART_BASE_PORT 0x3F8
#define UART_LINE_CONTROL_PARAMETERS (UART_LINE_CONTROL_8_BITS | UART_LINE_CONTROL_1_STOP_BIT | UART_LINE_CONTROL_NO_PARITY) //8N1


#define UART_DIVISOR (115200 / UART_BAUDRATE)
#define UART_DATA_PORT (UART_BASE_PORT + 0)
#define UART_DIVISOR_PORT (UART_BASE_PORT + 0)
#define UART_INTERRUPT_ENABLE_PORT (UART_BASE_PORT + 1)
#define UART_FIFO_CONTROL_PORT (UART_BASE_PORT + 2)
#define UART_LINE_CONTROL_PORT (UART_BASE_PORT + 3)
#define UART_MODEM_CONTROL_PORT (UART_BASE_PORT + 4)
#define UART_LINE_STATUS_PORT (UART_BASE_PORT + 5)

#define UART_TEST_MAGIC 0x5A
#define UART_TX_TIMEOUT (4 * 1000000 * 12) / (UART_BAUDRATE) //calculate timeout as 4 times the worst-case transmission time for given baudrate

static bool I686IsUartOwnedByHal = false;

STATUS HalDebugPortInit(void)
{
    IoPortWriteByte(UART_INTERRUPT_ENABLE_PORT, 0);
    IoPortWriteByte(UART_LINE_CONTROL_PORT, UART_LINE_CONTROL_DLAB);
    IoPortWriteByte(UART_DIVISOR_PORT, UART_DIVISOR & 0xFF);
    IoPortWriteByte(UART_DIVISOR_PORT + 1, UART_DIVISOR >> 8);
    IoPortWriteByte(UART_LINE_CONTROL_PORT, UART_LINE_CONTROL_PARAMETERS);
    IoPortWriteByte(UART_FIFO_CONTROL_PORT, UART_FIFO_ENABLE | UART_FIFO_CLEAR_RECEIVE | UART_FIFO_CLEAR_TRANSMIT | UART_FIFO_THRESHOLD_14_BYTES);
    
    IoPortWriteByte(UART_MODEM_CONTROL_PORT, UART_MODEM_CONTROL_LOOPBACK);
    IoPortWriteByte(UART_DATA_PORT, UART_TEST_MAGIC);

    uint64_t timeout = HalGetTimestampMicros() + UART_TX_TIMEOUT;
    while(!(IoPortReadByte(UART_LINE_STATUS_PORT) & UART_LINE_STATUS_DATA_READY))
    {
        if(HalGetTimestampMicros() > timeout)
            return DEVICE_NOT_AVAILABLE;
    }

    if(UART_TEST_MAGIC != IoPortReadByte(UART_DATA_PORT))
        return DEVICE_NOT_AVAILABLE;

    IoPortWriteByte(UART_MODEM_CONTROL_PORT, 0);

    I686IsUartOwnedByHal = true;
    return OK;
}

void I686DeinitIsaUart(void)
{
    I686IsUartOwnedByHal = false;
}

STATUS HalDebugPutChar(char c)
{
    if(!I686IsUartOwnedByHal)
        return DEVICE_NOT_AVAILABLE;

    uint64_t timeout = HalGetTimestampMicros() + UART_TX_TIMEOUT;
    while(!(IoPortReadByte(UART_LINE_STATUS_PORT) & UART_LINE_STATUS_THR_EMPTY))
    {
        if(HalGetTimestampMicros() > timeout)
            return TIMEOUT;
    }

    IoPortWriteByte(UART_DATA_PORT, (uint8_t)c);
    return OK;
}

STATUS HalDebugPutString(const char *str)
{
    if(!I686IsUartOwnedByHal)
        return DEVICE_NOT_AVAILABLE;

    STATUS status = OK;
    const char *s = str;
    while('\0' != *s)
    {
        status = HalDebugPutChar(*s);
        if(OK != status)
            return status;
        ++s;
    }
    return OK;
}

STATUS HalDebugPutStringN(const char *str, size_t n)
{
    if(!I686IsUartOwnedByHal)
        return DEVICE_NOT_AVAILABLE;

    STATUS status = OK;
    const char *s = str;
    size_t count = 0;
    while(('\0' != *s) && (count < n))
    {
        status = HalDebugPutChar(*s);
        if(OK != status)
            return status;
        ++s;
        ++count;
    }
    return OK;
}

STATUS HalDebugClearRxBuffer(void)
{
    if(!I686IsUartOwnedByHal)
        return DEVICE_NOT_AVAILABLE;

    IoPortWriteByte(UART_FIFO_CONTROL_PORT, UART_FIFO_ENABLE | UART_FIFO_CLEAR_RECEIVE | UART_FIFO_THRESHOLD_14_BYTES);

    return OK;
}

STATUS HalDebugClearTxBuffer(void)
{
    if(!I686IsUartOwnedByHal)
        return DEVICE_NOT_AVAILABLE;

    IoPortWriteByte(UART_FIFO_CONTROL_PORT, UART_FIFO_ENABLE | UART_FIFO_CLEAR_TRANSMIT | UART_FIFO_THRESHOLD_14_BYTES);

    return OK;
}

bool HalDebugIsDataAvailable(void)
{
    if(!I686IsUartOwnedByHal)
        return false;

    return !!(IoPortReadByte(UART_LINE_STATUS_PORT) & UART_LINE_STATUS_DATA_READY);
}


