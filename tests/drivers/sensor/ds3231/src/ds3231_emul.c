/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_ds3231_mfd

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "ds3231_emul.h"

#define DS3231_EMUL_REG_TEMP_MSB 0x11U
#define DS3231_EMUL_REG_TEMP_LSB 0x12U
#define DS3231_EMUL_NUM_REGS     0x13U

struct ds3231_emul_data {
	uint8_t regs[DS3231_EMUL_NUM_REGS];
	bool read_failure;
};

void ds3231_emul_set_temp_regs(const struct emul *target, uint8_t msb, uint8_t lsb)
{
	struct ds3231_emul_data *data = target->data;

	data->regs[DS3231_EMUL_REG_TEMP_MSB] = msb;
	data->regs[DS3231_EMUL_REG_TEMP_LSB] = lsb;
}

void ds3231_emul_set_read_failure(const struct emul *target, bool fail)
{
	struct ds3231_emul_data *data = target->data;

	data->read_failure = fail;
}

static int ds3231_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct ds3231_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	/* Only register reads are emulated: write the start register, then read */
	if ((num_msgs != 2) || ((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len != 1U) ||
	    ((msgs[1].flags & I2C_MSG_READ) == 0U)) {
		return -EIO;
	}

	reg = msgs[0].buf[0];
	if ((reg >= DS3231_EMUL_NUM_REGS) || (msgs[1].len > (DS3231_EMUL_NUM_REGS - reg))) {
		return -EIO;
	}

	if (data->read_failure) {
		return -EIO;
	}

	memcpy(msgs[1].buf, &data->regs[reg], msgs[1].len);

	return 0;
}

static int ds3231_emul_init(const struct emul *target, const struct device *parent)
{
	struct ds3231_emul_data *data = target->data;

	ARG_UNUSED(parent);
	memset(data, 0, sizeof(*data));

	return 0;
}

static const struct i2c_emul_api ds3231_emul_api = {
	.transfer = ds3231_emul_transfer,
};

#define DS3231_EMUL_DEFINE(inst)                                                                   \
	static struct ds3231_emul_data ds3231_emul_data_##inst;                                    \
	EMUL_DT_INST_DEFINE(inst, ds3231_emul_init, &ds3231_emul_data_##inst, NULL,                \
			    &ds3231_emul_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(DS3231_EMUL_DEFINE)
