/*
 * This file is part of the coreboot project.
 *
 * Copyright 2004 Tyan Computer
 *  by yhlu@tyan.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <types.h>
#include <console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "ck804.h"

static void usb2_init(struct device *dev)
{
	u32 dword;
	dword = pci_read_config32(dev, 0xf8);
	dword |= 40;
	pci_write_config32(dev, 0xf8, dword);
}

struct device_operations ck804_usb2_ops = {
	.id = {.type = DEVICE_ID_PCI,
		{.pci = {.vendor = PCI_VENDOR_ID_NVIDIA,
			 .device = PCI_DEVICE_ID_NVIDIA_CK804_USB2}}},
	.phase3_chip_setup_dev	 = ck804_enable,
	.phase3_scan		 = NULL,
	.phase4_read_resources	 = pci_dev_read_resources,
	.phase4_set_resources	 = pci_set_resources,
	.phase5_enable_resources = pci_dev_enable_resources,
	.phase6_init		 = usb2_init,
	.ops_pci		 = &ck804_ops_pci,
};

