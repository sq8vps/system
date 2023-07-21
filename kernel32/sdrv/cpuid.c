#include "cpuid.h"
#include <cpuid.h>

enum {
    CPUID_FEATURE_ECX_SSE3         = 1 << 0, 
    CPUID_FEATURE_ECX_PCLMUL       = 1 << 1,
    CPUID_FEATURE_ECX_DTES64       = 1 << 2,
    CPUID_FEATURE_ECX_MONITOR      = 1 << 3,  
    CPUID_FEATURE_ECX_DS_CPL       = 1 << 4,  
    CPUID_FEATURE_ECX_VMX          = 1 << 5,  
    CPUID_FEATURE_ECX_SMX          = 1 << 6,  
    CPUID_FEATURE_ECX_EST          = 1 << 7,  
    CPUID_FEATURE_ECX_TM2          = 1 << 8,  
    CPUID_FEATURE_ECX_SSSE3        = 1 << 9,  
    CPUID_FEATURE_ECX_CID          = 1 << 10,
    CPUID_FEATURE_ECX_SDBG         = 1 << 11,
    CPUID_FEATURE_ECX_FMA          = 1 << 12,
    CPUID_FEATURE_ECX_CX16         = 1 << 13, 
    CPUID_FEATURE_ECX_XTPR         = 1 << 14, 
    CPUID_FEATURE_ECX_PDCM         = 1 << 15, 
    CPUID_FEATURE_ECX_PCID         = 1 << 17, 
    CPUID_FEATURE_ECX_DCA          = 1 << 18, 
    CPUID_FEATURE_ECX_SSE4_1       = 1 << 19, 
    CPUID_FEATURE_ECX_SSE4_2       = 1 << 20, 
    CPUID_FEATURE_ECX_X2APIC       = 1 << 21, 
    CPUID_FEATURE_ECX_MOVBE        = 1 << 22, 
    CPUID_FEATURE_ECX_POPCNT       = 1 << 23, 
    CPUID_FEATURE_ECX_TSC          = 1 << 24, 
    CPUID_FEATURE_ECX_AES          = 1 << 25, 
    CPUID_FEATURE_ECX_XSAVE        = 1 << 26, 
    CPUID_FEATURE_ECX_OSXSAVE      = 1 << 27, 
    CPUID_FEATURE_ECX_AVX          = 1 << 28,
    CPUID_FEATURE_ECX_F16C         = 1 << 29,
    CPUID_FEATURE_ECX_RDRAND       = 1 << 30,
    CPUID_FEATURE_ECX_HYPERVISOR   = 1 << 31,
 
    CPUID_FEATURE_EDX_FPU          = 1 << 0,  
    CPUID_FEATURE_EDX_VME          = 1 << 1,  
    CPUID_FEATURE_EDX_DE           = 1 << 2,  
    CPUID_FEATURE_EDX_PSE          = 1 << 3,  
    CPUID_FEATURE_EDX_TSC          = 1 << 4,  
    CPUID_FEATURE_EDX_MSR          = 1 << 5,  
    CPUID_FEATURE_EDX_PAE          = 1 << 6,  
    CPUID_FEATURE_EDX_MCE          = 1 << 7,  
    CPUID_FEATURE_EDX_CX8          = 1 << 8,  
    CPUID_FEATURE_EDX_APIC         = 1 << 9,  
    CPUID_FEATURE_EDX_SEP          = 1 << 11, 
    CPUID_FEATURE_EDX_MTRR         = 1 << 12, 
    CPUID_FEATURE_EDX_PGE          = 1 << 13, 
    CPUID_FEATURE_EDX_MCA          = 1 << 14, 
    CPUID_FEATURE_EDX_CMOV         = 1 << 15, 
    CPUID_FEATURE_EDX_PAT          = 1 << 16, 
    CPUID_FEATURE_EDX_PSE36        = 1 << 17, 
    CPUID_FEATURE_EDX_PSN          = 1 << 18, 
    CPUID_FEATURE_EDX_CLFLUSH      = 1 << 19, 
    CPUID_FEATURE_EDX_DS           = 1 << 21, 
    CPUID_FEATURE_EDX_ACPI         = 1 << 22, 
    CPUID_FEATURE_EDX_MMX          = 1 << 23, 
    CPUID_FEATURE_EDX_FXSR         = 1 << 24, 
    CPUID_FEATURE_EDX_SSE          = 1 << 25, 
    CPUID_FEATURE_EDX_SSE2         = 1 << 26, 
    CPUID_FEATURE_EDX_SS           = 1 << 27, 
    CPUID_FEATURE_EDX_HTT          = 1 << 28, 
    CPUID_FEATURE_EDX_TM           = 1 << 29, 
    CPUID_FEATURE_EDX_IA64         = 1 << 30,
    CPUID_FEATURE_EDX_PBE          = 1 << 31
};

static bool cpuidAvailable = false;

bool CpuidInit(void)
{
    return CpuidCheckIfAvailable();
}

bool CpuidCheckIfAvailable(void)
{
    if(__get_cpuid_max(0, NULL))
    {
        cpuidAvailable = true;
        return true;
    }
    return false;
}

void CpuidGetVendorString(char *dst)
{
    if(!cpuidAvailable)
        return;
    uintptr_t dummy;
    __cpuid(0, dummy, dst[0], dst[8], dst[4]);
    dst[12] = '\0'; //null terminator
}

bool CpuidCheckIfApicAvailable(void)
{
    if(!cpuidAvailable)
        return false;
    uintptr_t dummy;
    uintptr_t reg;
    __cpuid(1, dummy, dummy, dummy, reg);
    return (reg & CPUID_FEATURE_EDX_APIC) > 0;
}

bool CpuidCheckIfMsrAvailable(void)
{
    if(!cpuidAvailable)
        return false;
    uintptr_t dummy;
    uintptr_t reg;
    __cpuid(1, dummy, dummy, dummy, reg);
    return (reg & CPUID_FEATURE_EDX_MSR) > 0;
}