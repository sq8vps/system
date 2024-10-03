#include "uuid.h"
#include "io/disp/print.h"
#include "common/order.h"

void RtlUuidConvertEndianess(void *uuid)
{
    uint8_t *u = uuid;
    uint64_t t16 = 0;
    uint64_t t64 = *((uint64_t*)(u + 8));
    t64 = __builtin_bswap64(t64);
    t16 = t64 & 0xFFFF;
    t64 >>= 16;
    t64 |= (t16 << 48);
    *((uint64_t*)(u + 8)) = t64;
}

void RtlUuidToString(const void *uuid, char *str, bool upperCase)
{
    const uint8_t *u = uuid;
    uint32_t p1 = CmBeU32(*(uint32_t*)u);
    uint16_t p2 = CmBeU16(*(uint16_t*)(u + 4));
    uint16_t p3 = CmBeU16(*(uint16_t*)(u + 6));
    uint16_t p4 = CmBeU16(*(uint16_t*)(u + 8));
    uint64_t p5 = CmBeU64(*(uint64_t*)(u + 8)) & 0xFFFFFFFFFFFF;
    if(upperCase)
        snprintf(str, RTL_UUID_STRING_LENGTH, "%08lX-%04hX-%04hX-%04hX-%012llX", p1, p2, p3, p4, p5);
    else
        snprintf(str, RTL_UUID_STRING_LENGTH, "%08lx-%04hx-%04hx-%04hx-%012llx", p1, p2, p3, p4, p5);
}