/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 * Copyright (C) 2015  Damien Zammit <damien@zamaudio.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define __SIMPLE_DEVICE__

#include <arch/cpu.h>
#include <device/pci_ops.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <console/console.h>
#include <cbmem.h>
#include <northbridge/intel/pineview/pineview.h>
#include <cpu/x86/mtrr.h>
#include <cpu/intel/romstage.h>
#include <cpu/intel/smm/gen1/smi.h>

u8 decode_pciebar(u32 *const base, u32 *const len)
{
	*base = 0;
	*len = 0;
	const pci_devfn_t dev = PCI_DEV(0,0,0);
	u32 pciexbar = 0;
	u32 pciexbar_reg;
	u32 reg32;
	int max_buses;
	const struct {
		u16 num_buses;
		u32 addr_mask;
	} busmask[] = {
		{256, 0xf0000000},
		{128, 0xf8000000},
		{64,  0xfc000000},
		{0,   0},
	};

	pciexbar_reg = pci_read_config32(dev, PCIEXBAR);

	// MMCFG not supported or not enabled.
	if (!(pciexbar_reg & (1 << 0))) {
		printk(BIOS_WARNING, "WARNING: MMCONF not set\n");
		return 0;
	}

	reg32 = (pciexbar_reg >> 1) & 3;
	pciexbar = pciexbar_reg & busmask[reg32].addr_mask;
	max_buses = busmask[reg32].num_buses;

	if (!pciexbar) {
		printk(BIOS_WARNING, "WARNING: pciexbar invalid\n");
		return 0;
	}

	*base = pciexbar;
	*len = max_buses << 20;
	return 1;
}

/** Decodes used Graphics Mode Select (GMS) to kilobytes. */
u32 decode_igd_memory_size(const u32 gms)
{
	const u32 gmssize[] = {
		0, 1, 4, 8, 16, 32, 48, 64, 128, 256
	};

	if (gms > 9) {
		printk(BIOS_DEBUG, "Bad Graphics Mode Select (GMS) value.\n");
		return 0;
	}
	return gmssize[gms] << 10;
}

/** Decodes used Graphics Stolen Memory (GSM) to kilobytes. */
u32 decode_igd_gtt_size(const u32 gsm)
{
	const u8 gsmsize[] = {
		0, 1, 0, 0,
	};

	if (gsm > 3) {
		printk(BIOS_DEBUG, "Bad Graphics Stolen Memory (GSM) value.\n");
		return 0;
	}
	return (u32)(gsmsize[gsm] << 10);
}

/** Decodes used TSEG size to bytes. */
static u32 decode_tseg_size(const u32 esmramc)
{
	if (!(esmramc & 1))
		return 0;

	switch ((esmramc >> 1) & 3) {
	case 0:
		return 1 << 20;
	case 1:
		return 2 << 20;
	case 2:
		return 8 << 20;
	case 3:
	default:
		die("Bad TSEG setting.\n");
	}
}

u32 northbridge_get_tseg_size(void)
{
	const u8 esmramc = pci_read_config8(PCI_DEV(0, 0, 0), ESMRAMC);
	return decode_tseg_size(esmramc);
}

u32 northbridge_get_tseg_base(void)
{
	return pci_read_config32(PCI_DEV(0, 0, 0), TSEG);
}


/* Depending of UMA and TSEG configuration, TSEG might start at any
 * 1 MiB alignment. As this may cause very greedy MTRR setup, push
 * CBMEM top downwards to 4 MiB boundary.
 */
void *cbmem_top(void)
{
	uintptr_t top_of_ram = ALIGN_DOWN(northbridge_get_tseg_base(), 4*MiB);
	return (void *) top_of_ram;

}

/* platform_enter_postcar() determines the stack to use after
 * cache-as-ram is torn down as well as the MTRR settings to use,
 * and continues execution in postcar stage. */
void platform_enter_postcar(void)
{
	struct postcar_frame pcf;
	uintptr_t top_of_ram;

	if (postcar_frame_init(&pcf, 0))
		die("Unable to initialize postcar frame.\n");

	/* Cache the ROM as WP just below 4GiB. */
	postcar_frame_add_romcache(&pcf, MTRR_TYPE_WRPROT);

	/* Cache RAM as WB from 0 -> CACHE_TMP_RAMTOP. */
	postcar_frame_add_mtrr(&pcf, 0, CACHE_TMP_RAMTOP, MTRR_TYPE_WRBACK);

	/* Cache 8 MiB region below the top of ram and 2 MiB above top of
	 * ram to cover both cbmem as the TSEG region.
	 */
	top_of_ram = (uintptr_t)cbmem_top();
	postcar_frame_add_mtrr(&pcf, top_of_ram - 8*MiB, 8*MiB,
			MTRR_TYPE_WRBACK);
	postcar_frame_add_mtrr(&pcf, northbridge_get_tseg_base(),
			       northbridge_get_tseg_size(), MTRR_TYPE_WRBACK);

	run_postcar_phase(&pcf);

	/* We do not return here. */
}
