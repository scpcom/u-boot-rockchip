/*
 * (C) Copyright 2017 Hardkernel Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <misc.h>
#include <ram.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <asm/setup.h>
#include <asm/arch/periph.h>
#include <power/regulator.h>
#include <u-boot/sha256.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <spl.h>
#include <linux/crc8.h>
#include <crc.h>

DECLARE_GLOBAL_DATA_PTR;

#define RK3399_CPUID_OFF  0x7
#define RK3399_CPUID_LEN  0x10

#define CPUID_SIZE	RK3399_CPUID_LEN

extern int board_scan_boot_storage(void);

void rk_setup_mac_addr(unsigned char *addr);

#ifdef CONFIG_BOARD_LATE_INIT
int rk_setup_boot_mode(void)
{
	if (board_scan_boot_storage() != 0) {
		printf("board: scan boot stoarge fail\n");
		return -1;
	}

	return 0;
}
#endif

int rk_board_init(void)
{
	struct udevice *pinctrl, *regulator;
	int ret;

	/*
	 * The PWM do not have decicated interrupt number in dts and can
	 * not get periph_id by pinctrl framework, so let's init them here.
	 * The PWM2 and PWM3 are for pwm regulater.
	 */
	ret = uclass_get_device(UCLASS_PINCTRL, 0, &pinctrl);
	if (ret) {
		debug("%s: Cannot find pinctrl device\n", __func__);
		goto out;
	}

	/* Enable pwm0 for panel backlight */
	ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_PWM0);
	if (ret) {
		debug("%s PWM0 pinctrl init fail! (ret=%d)\n", __func__, ret);
		goto out;
	}

	/* Enable pwm2 to control regulator vdd-log */
	ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_PWM2);
	if (ret) {
		debug("%s PWM2 pinctrl init fail!\n", __func__);
		goto out;
	}

	ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_PWM3);
	if (ret) {
		debug("%s PWM3 pinctrl init fail!\n", __func__);
		goto out;
	}

#if 0
	ret = regulators_enable_boot_on(false);
	if (ret)
		debug("%s: Cannot enable boot on regulator\n", __func__);
#endif

	ret = regulator_get_by_platname("vcc5v0_host", &regulator);
	if (ret) {
		debug("%s vcc5v0_host init fail! ret %d\n", __func__, ret);
		goto out;
	}

	ret = regulator_set_enable(regulator, true);
	if (ret) {
		debug("%s vcc5v0-host-en set fail!\n", __func__);
		goto out;
	}

	/* Enable regulator vdd_log to supply LOGIC_VDD on ODROID-N1 HW */
	ret = regulator_get_by_platname("vdd_log", &regulator);
	if (ret) {
		printf("%s vdd_log init fail! ret %d\n", __func__, ret);
		goto out;
	}

	ret = regulator_set_enable(regulator, true);
	if (ret) {
		printf("%s vdd_log set fail!\n", __func__);
		goto out;
	}

out:
	return 0;
}

void get_rockchip_cpuid(unsigned char *buf)
{
	int ret;
        struct udevice *dev;
        u8 cpuid[RK3399_CPUID_LEN];

        /* retrieve the device */
        ret = uclass_get_device_by_driver(UCLASS_MISC,
                                          DM_GET_DRIVER(rockchip_efuse), &dev);
        if (ret) {
                debug("%s: could not find efuse device\n", __func__);
                return;
        }

        /* read the cpu_id range from the efuses */
        ret = misc_read(dev, RK3399_CPUID_OFF, &cpuid, sizeof(cpuid));
        if (ret) {
                debug("%s: reading cpuid from the efuses failed\n",
                      __func__);
                return;
        }

	memcpy(buf, cpuid, RK3399_CPUID_LEN);
}

void rk_setup_mac_addr(unsigned char *addr)
{
	unsigned char cpuid[CPUID_SIZE];
	unsigned char low[CPUID_SIZE/2];
	unsigned char high[CPUID_SIZE/2];
	unsigned char i;
	unsigned int temp;

	/* get cpuid from soc rockchip driver */
	get_rockchip_cpuid(cpuid);

	/* rearrange cpuid as 8bytes unit */
	for (i = 0; i < (CPUID_SIZE / 2); i++) {
		low[i] = cpuid[1 + (i << 1)];
		high[i] = cpuid[i << 1];
	}

	/* calculate crc16 using low : 8byte input -> 2byte output */
	temp = crc16_ccitt(0, low, 8);

	/* calculate crc8 using high : 8byte input -> 1byte output */
	temp |= (u64)crc8(temp, high, 8) << 16;

	/* fixed pattern */
	addr[0] = 0x00;
	addr[1] = 0x1e;
	addr[2] = 0x06;

	/* unique pattern */
	addr[3] = (char)(0xff & (temp >> 16));
	addr[4] = (char)(0xff & (temp >> 8));
	addr[5] = (char)(0xff & temp);
}

static void setup_macaddr(void)
{
#if CONFIG_IS_ENABLED(CMD_NET)
	const char *cpuid = env_get("cpuid#");
	u8 mac_addr[6];

	/* Only generate a MAC address, if none is set in the environment */
	if (env_get("ethaddr"))
		return;

	if (!cpuid) {
		debug("%s: could not retrieve 'cpuid#'\n", __func__);
		return;
	}

	rk_setup_mac_addr(mac_addr);

	eth_env_set_enetaddr("ethaddr", mac_addr);
#endif

	return;
}

static void setup_serial(void)
{
#if CONFIG_IS_ENABLED(ROCKCHIP_EFUSE)
	struct udevice *dev;
	int ret, i;
	u8 cpuid[RK3399_CPUID_LEN];
	u8 low[RK3399_CPUID_LEN/2], high[RK3399_CPUID_LEN/2];
	char cpuid_str[RK3399_CPUID_LEN * 2 + 1];
	u64 serialno;
	char serialno_str[16];

	/* retrieve the device */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_GET_DRIVER(rockchip_efuse), &dev);
	if (ret) {
		debug("%s: could not find efuse device\n", __func__);
		return;
	}

	/* read the cpu_id range from the efuses */
	ret = misc_read(dev, RK3399_CPUID_OFF, &cpuid, sizeof(cpuid));
	if (ret) {
		debug("%s: reading cpuid from the efuses failed\n",
		      __func__);
		return;
	}

	memset(cpuid_str, 0, sizeof(cpuid_str));
	for (i = 0; i < 16; i++)
		sprintf(&cpuid_str[i * 2], "%02x", cpuid[i]);

	debug("cpuid: %s\n", cpuid_str);

	/*
	 * Mix the cpuid bytes using the same rules as in
	 *   ${linux}/drivers/soc/rockchip/rockchip-cpuinfo.c
	 */
	for (i = 0; i < 8; i++) {
		low[i] = cpuid[1 + (i << 1)];
		high[i] = cpuid[i << 1];
	}

	serialno = crc32_no_comp(0, low, 8);
	serialno |= (u64)crc32_no_comp(serialno, high, 8) << 32;
	snprintf(serialno_str, sizeof(serialno_str), "%llx", serialno);

	env_set("cpuid#", cpuid_str);
	env_set("serial#", serialno_str);
#endif

	return;
}

int misc_init_r(void)
{
	setup_serial();
	setup_macaddr();

	return 0;
}

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	char *serial_string;
	u64 serial = 0;

	serial_string = env_get("serial#");

	if (serial_string)
		serial = simple_strtoull(serial_string, NULL, 16);

	serialnr->high = (u32)(serial >> 32);
	serialnr->low = (u32)(serial & 0xffffffff);
}
#endif

#ifdef CONFIG_USB_DWC3
static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = 0xfe800000,
	.dr_mode = USB_DR_MODE_HOST,
	.index = 0,
	.dis_u2_susphy_quirk = 1,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

int board_usb_init(int index, enum usb_init_type init)
{
	return dwc3_uboot_init(&dwc3_device_data);
}
#endif
