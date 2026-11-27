/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_SENSOR_DS3231_EMUL_H_
#define ZEPHYR_TESTS_DRIVERS_SENSOR_DS3231_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

void ds3231_emul_set_temp_regs(const struct emul *target, uint8_t msb, uint8_t lsb);
void ds3231_emul_set_read_failure(const struct emul *target, bool fail);

#endif /* ZEPHYR_TESTS_DRIVERS_SENSOR_DS3231_EMUL_H_ */
