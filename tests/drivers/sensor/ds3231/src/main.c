/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "ds3231_emul.h"

#define DS3231_MFD    DT_NODELABEL(ds3231)
#define DS3231_SENSOR DT_NODELABEL(ds3231_sensor)

struct temp_vector {
	uint8_t msb;
	uint8_t lsb;
	int64_t micro_c;
};

/* Register values per the DS3231 datasheet, temperature registers 11h-12h */
static const struct temp_vector vectors[] = {
	{0x00, 0x00, 0},
	{0x19, 0x40, 25250000},
	{0x1F, 0xC0, 31750000},
	{0x20, 0x00, 32000000},
	{0x41, 0x00, 65000000},
	{0x55, 0x80, 85500000},
	{0x7F, 0xC0, 127750000},
	{0xFF, 0xC0, -250000},
	{0xFF, 0x00, -1000000},
	{0xE7, 0x40, -24750000},
	{0xD8, 0x00, -40000000},
	{0x80, 0x00, -128000000},
};

static const struct device *const sensor_dev = DEVICE_DT_GET(DS3231_SENSOR);
static const struct emul *const emul = EMUL_DT_GET(DS3231_MFD);

static void *ds3231_setup(void)
{
	zassert_true(device_is_ready(sensor_dev));
	zassert_not_null(emul);

	return NULL;
}

static void ds3231_before(void *fixture)
{
	ARG_UNUSED(fixture);
	ds3231_emul_set_read_failure(emul, false);
	ds3231_emul_set_temp_regs(emul, 0x00, 0x00);
}

ZTEST_SUITE(ds3231, NULL, ds3231_setup, ds3231_before, NULL, NULL);

static int64_t fetch_micro_c(void)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(sensor_dev));
	zassert_ok(sensor_channel_get(sensor_dev, SENSOR_CHAN_AMBIENT_TEMP, &val));
	zassert_true(((val.val1 >= 0) && (val.val2 >= 0)) || ((val.val1 <= 0) && (val.val2 <= 0)),
		     "val1 (%d) and val2 (%d) must have the same sign", val.val1, val.val2);

	return sensor_value_to_micro(&val);
}

ZTEST(ds3231, test_fetch_get_temperature)
{
	ARRAY_FOR_EACH_PTR(vectors, v) {
		ds3231_emul_set_temp_regs(emul, v->msb, v->lsb);
		zassert_equal(fetch_micro_c(), v->micro_c, "0x%02x/0x%02x", v->msb, v->lsb);
	}
}

ZTEST(ds3231, test_fetch_error_keeps_last_sample)
{
	struct sensor_value val;

	ds3231_emul_set_temp_regs(emul, 0x19, 0x40);
	zassert_equal(fetch_micro_c(), 25250000);

	ds3231_emul_set_read_failure(emul, true);
	zassert_not_ok(sensor_sample_fetch(sensor_dev));
	zassert_ok(sensor_channel_get(sensor_dev, SENSOR_CHAN_AMBIENT_TEMP, &val));
	zassert_equal(sensor_value_to_micro(&val), 25250000);
}

SENSOR_DT_READ_IODEV(ds3231_iodev, DS3231_SENSOR, {SENSOR_CHAN_AMBIENT_TEMP, 0});
/*
 * The completion is posted before the work queue thread frees its SQE, so back-to-back reads
 * need a second SQE while the previous one is still being released.
 */
RTIO_DEFINE(ds3231_rtio, 2, 2);

ZTEST(ds3231, test_read_decode_temperature)
{
	const struct sensor_chan_spec chan = {SENSOR_CHAN_AMBIENT_TEMP, 0};
	const struct sensor_decoder_api *decoder;
	struct sensor_q31_data data;
	uint8_t buf[32];

	zassert_ok(sensor_get_decoder(sensor_dev, &decoder));

	ARRAY_FOR_EACH_PTR(vectors, v) {
		uint32_t fit = 0;
		int64_t micro_c;

		ds3231_emul_set_temp_regs(emul, v->msb, v->lsb);
		zassert_ok(sensor_read(&ds3231_iodev, &ds3231_rtio, buf, sizeof(buf)));
		zassert_equal(decoder->decode(buf, chan, &fit, 1, &data), 1);

		micro_c = ((int64_t)data.readings[0].temperature * 1000000) >> (31 - data.shift);
		zassert_equal(micro_c, v->micro_c, "0x%02x/0x%02x", v->msb, v->lsb);
	}
}
