#include "ddk/ddk.h" //DDK includes
#include "ddk/gddk.h" //Graphic DDK includes
#include "tmvga.h" //driver routines

//driver metadata
KDRV_METADATA = {.name = "Text mode x86 VGA driver", .vendor = "OEM", .class = DDK_KDRVCLASS_SCREEN, .version = "1"};

//driver index to be filled
KDRV_INDEX_T index = 0;

//driver specialized function callbacks
GDDK_KDrvCallbacks_t functions =
{
	.textModeClear = Tmvga_clear,
	.textModeGetCursorPosition = Tmvga_getCursor,
	.textModePutChar = Tmvga_putChar,
	.textModeResetCursorPosition = Tmvga_resetCursor,
	.textModeSetCursorPosition = Tmvga_setCursor,
};

void start(void)
{
	Ex_registerDriverCallbacks(index, &functions);
}

void stop(void)
{
	Ex_registerDriverCallbacks(index, NULL);
}

//driver general functions callbacks
DDK_KDrvGeneralCallbacks_t generalFunctions = 
{
	.start = start,
	.stop = stop,
};

/**
 * @brief Driver entry (initialization)
 * @param idx Driver index assigned by kernel
*/
KDRV_ENTRY(idx)
{
	index = idx; //store index
	Ex_registerDriverGeneralCallbacks(index, &generalFunctions);
}



