/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __EVB_RK3399_H
#define __EVB_RK3399_H

#include <configs/rk3399_common.h>

#ifndef CONFIG_SPL_BUILD
#define CONFIG_MMC_SDHCI_SDMA
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV 0
/*
 * SPL @ 32k for ~128k
 * ENV @ 240k
 * u-boot @ 256K
 */
#undef CONFIG_ENV_OFFSET
#define CONFIG_ENV_OFFSET (240 * 1024)
#endif

#define SDRAM_BANK_SIZE			(2UL << 30)
#define CONFIG_MISC_INIT_R
#define CONFIG_SERIAL_TAG
#define CONFIG_ENV_OVERWRITE

#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_DNS
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_PXE
#define CONFIG_BOOTP_SUBNETMASK

#define CONFIG_CMDLINE_EDITING
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_LONGHELP
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_ENV_VARS_UBOOT_CONFIG

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS	\
	ROCKCHIP_DEVICE_SETTINGS \
	"setbootargs=setenv bootargs earlyprintk swiotlb=1 "		\
		"console=ttyFIQ0,115200n8 "				\
		"rw root=/dev/mmcblk1p2 rootfstype=ext4 rootwait "	\
		"storagemedia=${storagemedia}\0"			\
	"bootcmd=cfgload; run setbootargs; mmc dev ${bootdev}; "	\
		"load mmc ${bootdev} 0x02000000 Image; "		\
		"load mmc ${bootdev} 0x04000000 uInitrd; "		\
		"load mmc ${bootdev} 0x01f00000 rk3399-odroidn1-linux.dtb; "	\
		"booti 0x02000000 - 0x01f00000\0"

#ifndef CONFIG_BOARD_LATE_INIT
#define CONFIG_BOARD_LATE_INIT
#endif

#define CONFIG_BMP_16BPP
#define CONFIG_BMP_24BPP
#define CONFIG_BMP_32BPP

#define ROCKCHIP_DEVICE_SETTINGS \
		"stdout=serial,vidconsole\0" \
		"stderr=serial,vidconsole\0"

#ifndef CONFIG_DRM_ROCKCHIP_VIDEO_FRAMEBUFFER
#define CONFIG_DRM_ROCKCHIP_VIDEO_FRAMEBUFFER 1
#endif

#endif
