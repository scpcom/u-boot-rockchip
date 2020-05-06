/*
 * (C) Copyright 2017 PINE64
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/grf_rk3328.h>
#include <asm/arch/hardware.h>
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/pinctrl/rockchip.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SPL_BUILD)
#define GRF_BASE		0xFF100000
#define GPIO0_BASE  0xFF210000

enum {
	/* GPIO0D_IOMUX*/
	GPIO0D6_SEL_SHIFT	= 12,
	GPIO0D6_SEL_MASK	= 3 << GPIO0D6_SEL_SHIFT,
	GPIO0D6_GPIO		= 0,
	GPIO0D6_SDMMC0_PWRENM1	= 3,

	/* GPIO1A_IOMUX */
	GPIO1A0_SEL_SHIFT	= 0,
	GPIO1A0_SEL_MASK	= 0x3fff << GPIO1A0_SEL_SHIFT,
	GPIO1A0_CARD_DATA_CLK_CMD_DETN	= 0x1555,

	IOMUX_SEL_SDMMC_SHIFT	= 7,
	IOMUX_SEL_SDMMC_MASK	= 1 << IOMUX_SEL_SDMMC_SHIFT,
	IOMUX_SEL_SDMMC_M0	= 0,
	IOMUX_SEL_SDMMC_M1,
};

void board_init_sdmmc_pwr_en(void)
{
	struct rk3328_grf_regs * const grf = (void *)GRF_BASE;
	struct rockchip_gpio_regs * const gpio0 = (void *)GPIO0_BASE;

	printf("board_init_sdmmc_pwr_en\n");

	rk_clrsetreg(&grf->com_iomux,
		IOMUX_SEL_SDMMC_MASK,
		IOMUX_SEL_SDMMC_M1 << IOMUX_SEL_SDMMC_SHIFT);

	rk_clrsetreg(&grf->gpio0d_iomux,
		GPIO0D6_SEL_MASK,
		GPIO0D6_GPIO << GPIO0D6_SEL_SHIFT);

	rk_clrsetreg(&grf->gpio1a_iomux,
		GPIO1A0_SEL_MASK,
		GPIO1A0_CARD_DATA_CLK_CMD_DETN
		<< GPIO1A0_SEL_SHIFT);

	// set GPIO0_D6 to GPIO_ACTIVE_LOW
	clrsetbits_le32(&gpio0->swport_dr, 1UL << RK_PD6, 0);

	// set GPIO0_D6 to OUTPUT
	setbits_le32(&gpio0->swport_ddr, 1UL << RK_PD6);
}

int rk_board_init_f(void)
{
	board_init_sdmmc_pwr_en();

	return 0;
}
#endif

int rk_board_late_init(void)
{
	return 0;
}
