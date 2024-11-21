// SPDX-License-Identifier: GPL-2.0+
/*
 * AXP PMIC SPL driver
 * (C) Copyright 2024 Arm Ltd.
 */

#include <errno.h>
#include <linux/types.h>
#include <asm/arch/pmic_bus.h>
#include <axp_pmic.h>

struct axp_reg_desc_spl {
	u8	enable_reg;
	u8	enable_mask;
	u8	volt_reg;
	u8	volt_mask;
	u16	min_mV;
	u16	max_mV;
	u8	step_mV;
	u8	split;
};

#define NA 0xff

#if defined(CONFIG_AXP717_POWER)				/* AXP717 */

static const struct axp_reg_desc_spl axp_spl_dcdc_regulators[] = {
	{ 0x80, BIT(0), 0x83, 0x7f,  500, 1540,  10, 70 },
	{ 0x80, BIT(1), 0x84, 0x7f,  500, 1540,  10, 70 },
	{ 0x80, BIT(2), 0x85, 0x7f,  500, 1840,  10, 70 },
};

#define AXP_CHIP_VERSION	0x0
#define AXP_CHIP_VERSION_MASK	0x0
#define AXP_CHIP_ID		0x0
#define AXP_SHUTDOWN_REG	0x27
#define AXP_SHUTDOWN_MASK	BIT(0)

#elif defined(CONFIG_AXP313_POWER)				/* AXP313 */

static const struct axp_reg_desc_spl axp_spl_dcdc_regulators[] = {
	{ 0x10, BIT(0), 0x13, 0x7f,  500, 1540,  10, 70 },
	{ 0x10, BIT(1), 0x14, 0x7f,  500, 1540,  10, 70 },
	{ 0x10, BIT(2), 0x15, 0x7f,  500, 1840,  10, 70 },
};

#define AXP_CHIP_VERSION	0x3
#define AXP_CHIP_VERSION_MASK	0xc8
#define AXP_CHIP_ID		0x48
#define AXP_SHUTDOWN_REG	0x1a
#define AXP_SHUTDOWN_MASK	BIT(7)

#elif defined(CONFIG_AXP305_POWER)				/* AXP305 */

static const struct axp_reg_desc_spl axp_spl_dcdc_regulators[] = {
	{ 0x10, BIT(0), 0x12, 0x7f,  600, 1520,  10, 50 },
	{ 0x10, BIT(1), 0x13, 0x1f, 1000, 2550,  50, NA },
	{ 0x10, BIT(2), 0x14, 0x7f,  600, 1520,  10, 50 },
	{ 0x10, BIT(3), 0x15, 0x3f,  600, 1500,  20, NA },
	{ 0x10, BIT(4), 0x16, 0x1f, 1100, 3400, 100, NA },
};

#define AXP_CHIP_VERSION	0x3
#define AXP_CHIP_VERSION_MASK	0xcf
#define AXP_CHIP_ID		0x40
#define AXP_SHUTDOWN_REG	0x32
#define AXP_SHUTDOWN_MASK	BIT(7)

#else

	#error "Please define the regulator registers in axp_spl_regulators[]."

#endif

static u8 axp_mvolt_to_cfg(int mvolt, const struct axp_reg_desc_spl *reg)
{
	if (mvolt < reg->min_mV)
		mvolt = reg->min_mV;
	else if (mvolt > reg->max_mV)
		mvolt = reg->max_mV;

	mvolt -= reg->min_mV;

	/* voltage in the first range ? */
	if (mvolt <= reg->split * reg->step_mV)
		return mvolt / reg->step_mV;

	mvolt -= reg->split * reg->step_mV;

	return reg->split + mvolt / (reg->step_mV * 2);
}

static int axp_set_dcdc(int dcdc_num, unsigned int mvolt)
{
	const struct axp_reg_desc_spl *reg;
	int ret;

	if (dcdc_num < 1 || dcdc_num > ARRAY_SIZE(axp_spl_dcdc_regulators))
		return -EINVAL;

	reg = &axp_spl_dcdc_regulators[dcdc_num - 1];

	if (mvolt == 0)
		return pmic_bus_clrbits(reg->enable_reg, reg->enable_mask);

	ret = pmic_bus_write(reg->volt_reg, axp_mvolt_to_cfg(mvolt, reg));
	if (ret)
		return ret;

	return pmic_bus_setbits(reg->enable_reg, reg->enable_mask);
}

int axp_set_dcdc1(unsigned int mvolt)
{
	return axp_set_dcdc(1, mvolt);
}

int axp_set_dcdc2(unsigned int mvolt)
{
	return axp_set_dcdc(2, mvolt);
}

int axp_set_dcdc3(unsigned int mvolt)
{
	return axp_set_dcdc(3, mvolt);
}

int axp_set_dcdc4(unsigned int mvolt)
{
	return axp_set_dcdc(4, mvolt);
}

int axp_set_dcdc5(unsigned int mvolt)
{
	return axp_set_dcdc(5, mvolt);
}

int axp_init(void)
{
	int ret = pmic_bus_init();

	if (ret)
		return ret;

	if (AXP_CHIP_VERSION_MASK) {
		u8 axp_chip_id;

		ret = pmic_bus_read(AXP_CHIP_VERSION, &axp_chip_id);
		if (ret)
			return ret;

		if ((axp_chip_id & AXP_CHIP_VERSION_MASK) != AXP_CHIP_ID) {
			debug("unknown PMIC: 0x%x\n", axp_chip_id);
			return -EINVAL;
		}
	}

	return 0;
}

#if !CONFIG_IS_ENABLED(ARM_PSCI_FW) && !IS_ENABLED(CONFIG_SYSRESET_CMD_POWEROFF)
int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	pmic_bus_setbits(AXP_SHUTDOWN_REG, AXP_SHUTDOWN_MASK);

	/* infinite loop during shutdown */
	while (1)
		;

	/* not reached */
	return 0;
}
#endif


/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
// #include <common.h>
#include <errno.h>
#include <asm/arch/pmic_bus.h>
#include <axp_pmic.h>
// #include <axp313.h>


#ifndef __AXP313A_H__
#define __AXP313A_H__
//PMIC chip id reg03:bit7-6  bit3-
#define   AXP1530_CHIP_ID              (0x48)
#define   AXP313A_CHIP_ID              (0x4B)
#define   AXP313B_CHIP_ID              (0x4C)
#define AXP313A_DEVICE_ADDR			(0x3A3)
#ifndef CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#define AXP313A_RUNTIME_ADDR			(0x2d)
#else
#ifndef CONFIG_AXP313A_SUNXI_I2C_SLAVE
#define AXP313A_RUNTIME_ADDR			CONFIG_SYS_SUNXI_R_I2C0_SLAVE
#else
#define AXP313A_RUNTIME_ADDR                    CONFIG_AXP313A_SUNXI_I2C_SLAVE
#endif
#endif
/* define AXP313A REGISTER */
#define	AXP313A_POWER_ON_SOURCE_INDIVATION			(0x00)
#define	AXP313A_POWER_OFF_SOURCE_INDIVATION			(0x01)
#define	AXP313A_VERSION								(0x03)
#define	AXP313A_OUTPUT_POWER_ON_OFF_CTL				(0x10)
#define AXP313A_DCDC_DVM_PWM_CTL					(0x12)
#define	AXP313A_DC1OUT_VOL							(0x13)
#define	AXP313A_DC2OUT_VOL          				(0x14)
#define	AXP313A_DC3OUT_VOL          				(0x15)
#define	AXP313A_ALDO1OUT_VOL						(0x16)
#define	AXP313A_DLDO1OUT_VOL						(0x17)
#define	AXP313A_POWER_DOMN_SEQUENCE					(0x1A)
#define	AXP313A_PWROK_VOFF_SERT						(0x1B)
#define AXP313A_POWER_WAKEUP_CTL					(0x1C)
#define AXP313A_OUTPUT_MONITOR_CONTROL				(0x1D)
#define	AXP313A_POK_SET								(0x1E)
#define	AXP313A_IRQ_ENABLE							(0x20)
#define	AXP313A_IRQ_STATUS							(0x21)
#define AXP313A_WRITE_LOCK							(0x70)
#define AXP313A_ERROR_MANAGEMENT					(0x71)
#define	AXP313A_DCDC1_2_POWER_ON_DEFAULT_SET		(0x80)
#define	AXP313A_DCDC3_ALDO1_POWER_ON_DEFAULT_SET	(0x81)
#endif /* __AXP313A_REGS_H__ */

#ifdef PMU_DEBUG
#define axp_info(fmt...) printf("[axp][info]: " fmt)
#define axp_err(fmt...) printf("[axp][err]: " fmt)
#else
#define axp_info(fmt...)
#define axp_err(fmt...) printf("[axp][err]: " fmt)
#endif
typedef struct _axp_contrl_info
{
    char name[16];
    u32 min_vol;
    u32 max_vol;
    u32 cfg_reg_addr;
    u32 cfg_reg_mask;
    u32 step0_val;
    u32 split1_val;
    u32 step1_val;
    u32 ctrl_reg_addr;
    u32 ctrl_bit_ofs;
    u32 step2_val;
    u32 split2_val;
} axp_contrl_info;
__attribute__((section(".data"))) axp_contrl_info pmu_axp313a_ctrl_tbl[] = {
    /*name,    min,  max, reg,  mask, step0,split1_val, step1,ctrl_reg,ctrl_bit */
    {"dcdc1", 500, 3400, AXP313A_DC1OUT_VOL, 0x7f, 10, 1200, 20,
     AXP313A_OUTPUT_POWER_ON_OFF_CTL, 0, 100, 1540},
    {"dcdc2", 500, 1540, AXP313A_DC2OUT_VOL, 0x7f, 10, 1200, 20,
     AXP313A_OUTPUT_POWER_ON_OFF_CTL, 1},
    {"dcdc3", 500, 1840, AXP313A_DC3OUT_VOL, 0x7f, 10, 1200, 20,
     AXP313A_OUTPUT_POWER_ON_OFF_CTL, 2},
    {"aldo1", 500, 3500, AXP313A_ALDO1OUT_VOL, 0x1f, 100, 0, 0,
     AXP313A_OUTPUT_POWER_ON_OFF_CTL, 3},
    {"dldo1", 500, 3500, AXP313A_DLDO1OUT_VOL, 0x1f, 100, 0, 0,
     AXP313A_OUTPUT_POWER_ON_OFF_CTL, 4},
};
static axp_contrl_info *get_ctrl_info_from_tbl(char *name)
{
    int i = 0;
    int size = ARRAY_SIZE(pmu_axp313a_ctrl_tbl);
    axp_contrl_info *p;
    for (i = 0; i < size; i++)
    {
        if (!strncmp(name, pmu_axp313a_ctrl_tbl[i].name,
                     strlen(pmu_axp313a_ctrl_tbl[i].name)))
        {
            break;
        }
    }
    if (i >= size)
    {
        axp_err("can't find %s from table\n", name);
        return NULL;
    }
    p = pmu_axp313a_ctrl_tbl + i;
    return p;
}
int pmu_axp313a_necessary_reg_enable(void)
{
    __attribute__((unused)) u8 reg_value;
#ifdef CONFIG_AXP313A_NECESSARY_REG_ENABLE
    if (pmic_bus_read(AXP313A_RUNTIME_ADDR, AXP313A_WRITE_LOCK, &reg_value))
        return -1;
    reg_value |= 0x5;
    if (pmic_bus_write(AXP313A_RUNTIME_ADDR, AXP313A_WRITE_LOCK, reg_value))
        return -1;
    if (pmic_bus_read(AXP313A_RUNTIME_ADDR, AXP313A_ERROR_MANAGEMENT, &reg_value))
        return -1;
    reg_value |= 0x8;
    if (pmic_bus_write(AXP313A_RUNTIME_ADDR, AXP313A_ERROR_MANAGEMENT, reg_value))
        return -1;
    if (pmic_bus_read(AXP313A_RUNTIME_ADDR, AXP313A_DCDC_DVM_PWM_CTL, &reg_value))
        return -1;
    reg_value |= (0x1 << 5);
    if (pmic_bus_write(AXP313A_RUNTIME_ADDR, AXP313A_DCDC_DVM_PWM_CTL, reg_value))
        return -1;
#endif
    return 0;
}

int pmu_axp313a_get_info(char *name, unsigned char *chipid)
{
    strncpy(name, "axp313a", sizeof("axp313a"));
    *chipid = AXP313A_CHIP_ID;
    return 0;
}
int pmu_axp313a_set_voltage(char *name, uint set_vol, uint onoff)
{
    u8 reg_value;
    axp_contrl_info *p_item = NULL;
    u8 base_step = 0;
    p_item = get_ctrl_info_from_tbl(name);
    if (!p_item)
    {
        return -1;
    }
    axp_info(
        "name %s, min_vol %dmv, max_vol %d, cfg_reg 0x%x, cfg_mask 0x%x \
		step0_val %d, split1_val %d, step1_val %d, ctrl_reg_addr 0x%x, ctrl_bit_ofs %d\n",
        p_item->name, p_item->min_vol, p_item->max_vol,
        p_item->cfg_reg_addr, p_item->cfg_reg_mask, p_item->step0_val,
        p_item->split1_val, p_item->step1_val, p_item->ctrl_reg_addr,
        p_item->ctrl_bit_ofs);
    if ((set_vol > 0) && (p_item->min_vol))
    {
        if (set_vol < p_item->min_vol)
        {
            set_vol = p_item->min_vol;
        }
        else if (set_vol > p_item->max_vol)
        {
            set_vol = p_item->max_vol;
        }
        if (pmic_bus_read(p_item->cfg_reg_addr,
                          &reg_value))
        {
            return -1;
        }
        reg_value &= ~p_item->cfg_reg_mask;
        if (p_item->split2_val && (set_vol > p_item->split2_val))
        {
            base_step = (p_item->split2_val - p_item->split1_val) /
                        p_item->step1_val;
            base_step += (p_item->split1_val - p_item->min_vol) /
                         p_item->step0_val;
            reg_value |= (base_step +
                          (set_vol - p_item->split2_val / p_item->step2_val * p_item->step2_val) /
                              p_item->step2_val);
        }
        else if (p_item->split1_val &&
                 (set_vol > p_item->split1_val))
        {
            if (p_item->split1_val < p_item->min_vol)
            {
                axp_err("bad split val(%d) for %s\n",
                        p_item->split1_val, name);
            }
            base_step = (p_item->split1_val - p_item->min_vol) /
                        p_item->step0_val;
            reg_value |= (base_step +
                          (set_vol - p_item->split1_val) /
                              p_item->step1_val);
        }
        else
        {
            reg_value |=
                (set_vol - p_item->min_vol) / p_item->step0_val;
        }
        if (pmic_bus_write(p_item->cfg_reg_addr, reg_value))
        {
            axp_err("unable to set %s\n", name);
            return -1;
        }
    }
    if (onoff < 0)
    {
        return 0;
    }
    if (pmic_bus_read(p_item->ctrl_reg_addr, &reg_value))
    {
        return -1;
    }
    if (onoff == 0)
    {
        reg_value &= ~(1 << p_item->ctrl_bit_ofs);
    }
    else
    {
        reg_value |= (1 << p_item->ctrl_bit_ofs);
    }
    if (pmic_bus_write(p_item->ctrl_reg_addr, reg_value))
    {
        axp_err("unable to onoff %s\n", name);
        return -1;
    }
    return 0;
}
int pmu_axp313a_get_voltage(char *name)
{
    u8 reg_value;
    axp_contrl_info *p_item = NULL;
    u8 base_step;
    int vol;
    p_item = get_ctrl_info_from_tbl(name);
    if (!p_item)
    {
        return -1;
    }
    if (pmic_bus_read(p_item->ctrl_reg_addr, &reg_value))
    {
        return -1;
    }
    if (!(reg_value & (0x01 << p_item->ctrl_bit_ofs)))
    {
        return 0;
    }
    if (pmic_bus_read(p_item->cfg_reg_addr, &reg_value))
    {
        return -1;
    }
    reg_value &= p_item->cfg_reg_mask;
    if (p_item->split2_val)
    {
        u32 base_step2;
        base_step = (p_item->split1_val - p_item->min_vol) /
                    p_item->step0_val;
        base_step2 = base_step + (p_item->split2_val - p_item->split1_val) /
                                     p_item->step1_val;
        if (reg_value >= base_step2)
        {
            vol = ALIGN(p_item->split2_val, p_item->step2_val) +
                  p_item->step2_val * (reg_value - base_step2);
        }
        else if (reg_value >= base_step)
        {
            vol = p_item->split1_val +
                  p_item->step1_val * (reg_value - base_step);
        }
        else
        {
            vol = p_item->min_vol + p_item->step0_val * reg_value;
        }
    }
    else if (p_item->split1_val)
    {
        base_step = (p_item->split1_val - p_item->min_vol) /
                    p_item->step0_val;
        if (reg_value > base_step)
        {
            vol = p_item->split1_val +
                  p_item->step1_val * (reg_value - base_step);
        }
        else
        {
            vol = p_item->min_vol + p_item->step0_val * reg_value;
        }
    }
    else
    {
        vol = p_item->min_vol + p_item->step0_val * reg_value;
    }
    return vol;
}
int pmu_axp313a_set_power_off(void)
{
    u8 reg_value;
    if (pmic_bus_read(AXP313A_POWER_DOMN_SEQUENCE, &reg_value))
    {
        return -1;
    }
    reg_value |= (1 << 7);
    if (pmic_bus_write(AXP313A_POWER_DOMN_SEQUENCE, reg_value))
    {
        return -1;
    }
    return 0;
}
int pmu_axp313a_get_key_irq(void)
{
    u8 reg_value;
    if (pmic_bus_read(AXP313A_IRQ_STATUS, &reg_value))
    {
        return -1;
    }
    reg_value &= (0x03 << 4);
    if (reg_value)
    {
        if (pmic_bus_write(AXP313A_IRQ_STATUS, reg_value))
        {
            return -1;
        }
    }
    return (reg_value >> 4) & 3;
}
unsigned char pmu_axp313a_get_reg_value(unsigned char reg_addr)
{
    u8 reg_value;
    if (pmic_bus_read(reg_addr, &reg_value))
    {
        return -1;
    }
    return reg_value;
}
unsigned char pmu_axp313a_set_reg_value(unsigned char reg_addr, unsigned char reg_value)
{
    unsigned char reg;
    if (pmic_bus_write(reg_addr, reg_value))
    {
        return -1;
    }
    if (pmic_bus_read(reg_addr, &reg))
    {
        return -1;
    }
    return reg;
}
