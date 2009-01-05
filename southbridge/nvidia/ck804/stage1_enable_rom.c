/*
 * Copyright 2004 Tyan Computer
 *  by yhlu@tyan.com
 */

#include <mainboard.h>
#include <types.h>
#include <device/pci.h>

#if HT_CHAIN_END_UNITID_BASE < HT_CHAIN_UNITID_BASE
#define CK804_DEVN_BASE HT_CHAIN_END_UNITID_BASE
#else
#define CK804_DEVN_BASE HT_CHAIN_UNITID_BASE
#endif

void ck804_enable_rom(void)
{
	unsigned char byte;
	u32 addr;

	/* Enable 4MB ROM access at 0xFFC00000 - 0xFFFFFFFF. */
	/* Locate the ck804 LPC. */
	addr = PCI_BDEVFN(0, PCI_DEVFN((CK804_DEVN_BASE + 1), 0));

	/* Set the 4MB enable bit. */
	byte = pci_conf1_read_config8(addr, 0x88);
	byte |= 0x80;
	pci_conf1_write_config8(addr, 0x88, byte);
}
