#include "pci.h"

#define PCI_CONFIG_ADDR 0xcf8
#define PCI_CONFIG_DATA 0xcfc


void pci_checkDevice(uint8_t bus, uint8_t dev);
void pci_scanBus(uint8_t bus);
void pci_checkFunction(Pci_s_t pci);

void (*pci_ataEnumCallback)(Pci_s_t, uint32_t) = (void*)0;
void (*pci_sataEnumCallback)(Pci_s_t, uint32_t) = (void*)0;

/**
 * \brief Reads dword from the configuation register
 * \param pci PCI structure
 * \param offset Offset of the word to be read (in bytes)
 * \return Data read
 */
static uint32_t pci_configReadDword(Pci_s_t pci, uint8_t offset)
{
	pci.dev &= 0b11111;
	pci.func &= 0b111;
	uint32_t a = (((uint32_t)pci.bus << 16) | ((uint32_t)pci.dev << 11) | ((uint32_t)pci.func << 8) | (offset & 0b11111100) | 0x80000000); //last thing is the enable bit
	port_writeDword(PCI_CONFIG_ADDR, a);
	return (port_readDword(PCI_CONFIG_DATA)); //read data
}

/**
 * \brief Writes dword to the configuation register
 * \param pci PCI structure
 * \param offset Offset of the word to be read (in bytes)
 */
static void pci_configWriteDword(Pci_s_t pci, uint8_t offset, uint32_t data)
{
	pci.dev &= 0b11111;
	pci.func &= 0b111;
	uint32_t a = (((uint32_t)pci.bus << 16) | ((uint32_t)pci.dev << 11) | ((uint32_t)pci.func << 8) | (offset & 0b11111100) | 0x80000000); //last thing is the enable bit
	port_writeDword(PCI_CONFIG_ADDR, a);
	port_writeDword(PCI_CONFIG_DATA, data);
}


/**
 * \brief Reads word from the configuation register
 * \param pci PCI structure
 * \param offset Offset of the word to be read (in bytes)
 * \return Data read
 */
static uint16_t pci_configReadWord(Pci_s_t pci, uint8_t offset)
{
	pci.dev &= 0b11111;
	pci.func &= 0b111;
	uint32_t a = (((uint32_t)pci.bus << 16) | ((uint32_t)pci.dev << 11) | ((uint32_t)pci.func << 8) | (offset & 0b11111100) | 0x80000000); //last thing is the enable bit
	port_writeDword(PCI_CONFIG_ADDR, a);
	return ((port_readDword(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF); //read data and select only the required field
}

/**
 * \brief Reads byte from the configuation register
 * \param pci PCI structure
 * \param offset Offset of the byte to be read (in bytes)
 * \return Data read
 */
uint8_t pci_configReadByte(Pci_s_t pci, uint8_t offset)
{
	pci.dev &= 0b11111;
	pci.func &= 0b111;
	uint32_t a = (((uint32_t)pci.bus << 16) | ((uint32_t)pci.dev << 11) | ((uint32_t)pci.func << 8) | (offset & 0b11111100) | 0x80000000); //last thing is the enable bit
	port_writeDword(PCI_CONFIG_ADDR, a);
	return ((port_readDword(PCI_CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF); //read data and select only the required field
}


/**
 * \brief Reads vendor ID
 * \param pci PCI structure
 * \return Vendor ID or PCI_VENDOR_INVALID if device non-existent
 */
uint16_t pci_getVendor(Pci_s_t pci)
{
	return pci_configReadWord(pci, PCI_STD_VENID); //vendor ID is always at the same offset (header type doesn't matter)
}

/**
 * \brief Reads device ID
 * \param pci PCI structure
 * \return Device ID or PCI_DEVICE_INVALID if device non-existent
 */
uint16_t pci_getDevice(Pci_s_t pci)
{
	return pci_configReadWord(pci, PCI_STD_DEVID); //device ID is always at the same offset (header type doesn't matter)
}


/**
 * \brief Reads header type
 * \param pci PCI structure
 * \return Header type or PCI_HEADER_INVALID if device non-existent
 */
uint8_t pci_getHeaderType(Pci_s_t pci)
{
	return pci_configReadByte(pci, PCI_STD_HEADERTYPE); //header type is always at the same offset (header type doesn't matter)
}

/**
 * \brief Reads device class
 * \param pci PCI structure
 * \return Device class or PCI_CLASS_INVALID if device non-existent
 */
uint8_t pci_getClass(Pci_s_t pci)
{
	return pci_configReadByte(pci, PCI_STD_CLASS); //class is always at the same offset (header type doesn't matter)
}


/**
 * \brief Reads device subclass
 * \param pci PCI structure
 * \return Device subclass or PCI_SUBCLASS_INVALID if device non-existent
 */
uint8_t pci_getSubClass(Pci_s_t pci)
{
	return pci_configReadByte(pci, PCI_STD_SUBCLASS); //subclass is always at the same offset (header type doesn't matter)
}

/**
 * \brief Writes data to selected BARx register
 * \param pci PCI structure
 * \param bar BARx register number (x: 0-5)
 * \param data Data to be written
 */
void pci_setBAR(Pci_s_t pci, uint8_t bar, uint32_t data)
{
	if(bar > 5) bar = 5;
	pci_configWriteDword(pci, PCI_STD_BAR0 + (bar * 4), data);
}

/**
 * \brief Reads programming inteface data
 * \param pci PCI structure
 * \return Programming interface data or PCI_PROGIF_INVALID if device non-existent
 */
uint8_t pci_getProgIf(Pci_s_t pci)
{
	return pci_configReadByte(pci, PCI_STD_PROGIF);
}

/**
 * \brief Writes to IDE command register
 * \param pci PCI structure
 * \param data Data to be written
 */
void pci_setIdeCommand(Pci_s_t pci, uint16_t data)
{
	uint32_t t = pci_configReadDword(pci, PCI_STD_CMD) & 0xFFFFFFFA; //get current register value
	t = t | (uint32_t)(data & 0b101);
	pci_configWriteDword(pci, PCI_STD_CMD, t);
}



/**
 * \brief Sets/resets IDE Decode Enable bit
 * \param pci PCI structure
 * \param primsec 0 - primary channel, 1 - secondary channel
 * \param state 0 - reset, 1 - set
 */
void pci_setIdeDecode(Pci_s_t pci, uint8_t primsec, uint16_t state)
{
	if(primsec > 1) return;
	uint32_t t = pci_configReadDword(pci, PCI_IDE_TIMING_PRIMARY);
	if(primsec == 0)
	{
		t = (t & 0xFFFF0000) | (uint32_t)((state > 0) << 15);
		pci_configWriteDword(pci, PCI_IDE_TIMING_PRIMARY, t);
	}
	else
	{
		t = (t & 0xFFFF) | (uint32_t)((state > 0) << 31);
		pci_configWriteDword(pci, PCI_IDE_TIMING_PRIMARY, t);
	}
}

/**
 * \brief Checks device and calls function check
 * \param bus Bus number
 * \param dev Device number
 */
void pci_checkDevice(uint8_t bus, uint8_t dev)
{
	Pci_s_t pci = {bus, dev, 0};
	if(pci_getVendor(pci) == PCI_VENDOR_INVALID) 
		return; //this device doesn't exist
	pci_checkFunction(pci); //the device must have at least 1 function, so check it
	if(pci_getHeaderType(pci) & PCI_HEADER_MF_BIT) //if the device has the multi-function (MF) bit set, it provides more than 1 function
	{
		for(uint8_t i = 1; i < 8; i++) //check remaining functions
		{
			pci.func = i;
			if(pci_getVendor(pci) != PCI_VENDOR_INVALID) //check function only if it's present
				pci_checkFunction(pci);
		}
	}
}

/**
 * \brief Scans specified bus
 * \param bus Bus number
 */
void pci_scanBus(uint8_t bus)
{
	for(uint8_t i = 0; i < 32; i++)
	{
		pci_checkDevice(bus, i);
	}
}

/**
 * \brief Checks device function and handles it
 * \param pci PCI structure
 */
void pci_checkFunction(Pci_s_t pci)
{
	uint8_t class = pci_getClass(pci);
	uint8_t subclass = pci_getSubClass(pci);

#if __DEBUG > 1
	printf("\nPCI at bus %d, dev %d, func %d: ven 0x%X, dev 0x%X, class 0x%X, subclass 0x%X", (int)pci.bus, (int)pci.dev, (int)pci.func, (int)pci_getVendor(pci), (int)pci_getDevice(pci), (int)class, (int)subclass);
#endif

	if((class == PCI_CLASS_BRIDGE) && (subclass == PCI_SUBCLASS_PCI_PCI)) //the device is a PCI-PCI bridge
	{
		//if this is a PCI-PCI bridge, we must get a number of the bus which is on the other side of this bridge
		//and then scan that bus
		pci_scanBus(pci_configReadByte(pci, PCI_BRIDGE_SECBUS));
	}
	if(class == PCI_CLASS_STORAGE) //the device is a storage controller
	{
		if(subclass == PCI_SUBCLASS_IDE) //subclass for PATA controllers or SATA working in compatibility mode
		{
			if(pci_ataEnumCallback != 0)
			{
				(*pci_ataEnumCallback)(pci, pci_configReadDword(pci, PCI_STD_VENID));
			}
		}
		else if(subclass == PCI_SUBCLASS_SATA) //subclass used for AHCI (native) SATA mode
		{
			if(pci_sataEnumCallback != 0)
			{
				(*pci_sataEnumCallback)(pci, pci_configReadDword(pci, PCI_STD_VENID));
			}
		}
	}
}

/**
 * \brief Scans all buses
 * \attention This is the only PCI enumeration function that can be called from the outside.
 */
void pci_scanAll(void)
{
	Pci_s_t pci = {0 ,0 ,0};
	//check if the very first device is a multifunction device. If so, there are more PCI host controllers. If not, this is the only host controller.
	if(pci_getHeaderType(pci) & PCI_HEADER_MF_BIT) //multifunction device
	{
		for(uint8_t i = 0; i < 8; i++) //loop through every bus
		{
			pci.func = i;
			if(pci_getVendor(pci) != PCI_VENDOR_INVALID) //if this function exists
			{
				pci_scanBus(i);
			}
		}
	}
	else //single-function device
	{
		pci_scanBus(0);
	}
}
/**
 * \brief Sets callback function for ATA controller enumeration
 * \param *f Callback function pointer, argument 0 is a PCI structure, argument 1 is a 32-bit dword containing vendor ID and device ID
 */
void pci_setAtaEnumCallback(void (*f)(Pci_s_t, uint32_t))
{
	pci_ataEnumCallback = f;
}

/**
 * \brief Sets callback function for SATA AHCI controller enumeration
 * \param *f Callback function pointer, argument 0 is a PCI structure, argument 1 is a 32-bit dword containing vendor ID and device ID
 */
void pci_setSataEnumCallback(void (*f)(Pci_s_t, uint32_t))
{
	pci_sataEnumCallback = f;
}
