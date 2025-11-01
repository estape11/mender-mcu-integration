// Copyright 2024 Northern.tech AS
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mender_app, LOG_LEVEL_DBG);

#include "utils/netup.h"
#include "utils/certs.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "mender/client.h"
#include "mender/inventory.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/hwinfo.h>

#define STRIP_NODE		DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)

#define SLEEP_TIME_MS 1000

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb O = RGB(0x00, 0x00, 0x00);
static const struct led_rgb W = RGB(0xff, 0xff, 0xff);

const struct led_rgb pixels_off[STRIP_NUM_PIXELS] = {
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O
};

const struct led_rgb pixels_boot[STRIP_NUM_PIXELS] = {
    O, O, O, O, O, O, O, O,
    O, O, O, W, W, O, O, O,
    O, O, W, O, O, W, O, O,
    O, W, O, O, O, O, W, O,
    O, W, O, O, O, O, W, O,
    O, O, W, O, O, W, O, O,
    O, O, O, W, W, O, O, O,
    O, O, O, O, O, O, O, O
};

// To configure which pixel art to use
#ifndef MATRIX_ART
#define MATRIX_ART 1
#endif

#if MATRIX_ART == 1

static const struct led_rgb R = RGB(0x0f, 0x00, 0x00);
static const struct led_rgb Y = RGB(0x0f, 0x0f, 0x00);

// Smile
const struct led_rgb pixels_payload1[STRIP_NUM_PIXELS] = {
    O, O, Y, Y, Y, Y, O, O,
    O, Y, Y, Y, Y, Y, Y, O,
    Y, Y, W, W, Y, R, Y, Y,
    Y, R, Y, Y, Y, Y, Y, Y,
    Y, Y, Y, Y, Y, Y, R, Y,
    Y, Y, R, Y, W, W, Y, Y,
    O, Y, Y, Y, Y, Y, Y, O,
    O, O, Y, Y, Y, Y, O, O
};

const struct led_rgb pixels_payload2[STRIP_NUM_PIXELS] = {
    R, R, Y, Y, Y, Y, R, R,
    R, Y, Y, Y, Y, Y, Y, R,
    Y, Y, W, W, Y, R, Y, Y,
    Y, R, Y, Y, Y, Y, Y, Y,
    Y, Y, Y, Y, Y, Y, R, Y,
    Y, Y, R, Y, W, W, Y, Y,
    R, Y, Y, Y, Y, Y, Y, R,
    R, R, Y, Y, Y, Y, R, R
};

#elif MATRIX_ART == 2

static const struct led_rgb R = RGB(0x0f, 0x00, 0x00);

// Heart
const struct led_rgb pixels_payload1[STRIP_NUM_PIXELS] = {
    O, O, R, R, R, O, O, O,
    O, O, R, R, R, R, R, O,
    O, R, R, R, R, R, R, O,
    R, R, R, R, R, R, O, O,
    O, O, R, R, R, R, R, R,
    O, R, R, R, R, R, R, O,
    O, R, R, R, R, R, O, O,
    O, O, O, R, R, R, O, O
};

const struct led_rgb pixels_payload2[STRIP_NUM_PIXELS] = {
    R, R, O, O, O, R, R, R,
    R, R, O, O, O, O, O, R,
    R, O, O, O, O, O, O, R,
    O, O, O, O, O, O, R, R,
    R, R, O, O, O, O, O, O,
    R, O, O, O, O, O, O, R,
    R, O, O, O, O, O, R, R,
    R, R, R, O, O, O, R, R
};

#elif MATRIX_ART == 3

static const struct led_rgb M = RGB(0x80, 0x80, 0x80);
static const struct led_rgb R = RGB(0x0f, 0x00, 0x00);
static const struct led_rgb G = RGB(0x00, 0x0f, 0x00);
static const struct led_rgb B = RGB(0x00, 0x00, 0x0f);
static const struct led_rgb Y = RGB(0x0f, 0x0f, 0x00);

// Rainbow
const struct led_rgb pixels_payload1[STRIP_NUM_PIXELS] = {
    R, R, R, R, R, R, R, R,
    Y, Y, Y, Y, Y, Y, Y, Y,
    G, G, G, G, G, G, G, G,
    B, B, B, B, B, B, B, B,
    M, M, M, M, M, M, M, M,
    R, R, R, R, R, R, R, R,
    Y, Y, Y, Y, Y, Y, Y, Y,
    G, G, G, G, G, G, G, G
};

const struct led_rgb pixels_payload2[STRIP_NUM_PIXELS] = {
    Y, Y, Y, Y, Y, Y, Y, Y,
    G, G, G, G, G, G, G, G,
    R, R, R, R, R, R, R, R,
    Y, Y, Y, Y, Y, Y, Y, Y,
    G, G, G, G, G, G, G, G,
    B, B, B, B, B, B, B, B,
    M, M, M, M, M, M, M, M,
    R, R, R, R, R, R, R, R
};

#else  // MATRIX_ART != 1

static const struct led_rgb R = RGB(0x0f, 0x00, 0x00);

const struct led_rgb pixels_payload1[STRIP_NUM_PIXELS] = {
    R, R, R, W, W, R, R, R,
    R, R, R, W, W, R, R, R,
    R, R, R, W, W, R, R, R,
    W, W, W, W, W, W, W, W,
    W, W, W, W, W, W, W, W,
    R, R, R, W, W, R, R, R,
    R, R, R, W, W, R, R, R,
    R, R, R, W, W, R, R, R
};

const struct led_rgb pixels_payload2[STRIP_NUM_PIXELS] = {
    W, W, W, R, R, W, W, W,
    W, W, W, R, R, W, W, W,
    W, W, W, R, R, W, W, W,
    R, R, R, R, R, R, R, R,
    R, R, R, R, R, R, R, R,
    W, W, W, R, R, W, W, W,
    W, W, W, R, R, W, W, W,
    W, W, W, R, R, W, W, W
};

#endif  // MATRIX_ART

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

#ifdef BUILD_INTEGRATION_TESTS
#include "modules/test-update-module.h"
#include "test_definitions.h"
#endif /* BUILD_INTEGRATION_TESTS */

#ifdef CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE
#include <mender/zephyr-image-update-module.h>
#endif /* CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE */

#ifdef CONFIG_MENDER_APP_NOOP_UPDATE_MODULE
#include "modules/noop-update-module.h"
#endif /* CONFIG_MENDER_APP_NOOP_UPDATE_MODULE */

#ifdef CONFIG_MENDER_CLIENT_INVENTORY_DISABLE
#error Mender MCU integration app requires the inventory feature
#endif /* CONFIG_MENDER_CLIENT_INVENTORY_DISABLE */

MENDER_FUNC_WEAK mender_err_t
mender_network_connect_cb(void) {
    LOG_DBG("network_connect_cb");
    return MENDER_OK;
}

MENDER_FUNC_WEAK mender_err_t
mender_network_release_cb(void) {
    LOG_DBG("network_release_cb");
    return MENDER_OK;
}

MENDER_FUNC_WEAK mender_err_t
mender_deployment_status_cb(mender_deployment_status_t status, const char *desc) {
    LOG_DBG("deployment_status_cb: %s", desc);
    return MENDER_OK;
}

MENDER_FUNC_WEAK mender_err_t
mender_restart_cb(void) {
    LOG_DBG("restart_cb");

    sys_reboot(SYS_REBOOT_WARM);

    return MENDER_OK;
}

static char              chip_id[18] = { 0 };
static mender_identity_t mender_identity = { .name = "chip_id", .value = chip_id };

MENDER_FUNC_WEAK mender_err_t
mender_get_identity_cb(const mender_identity_t **identity) {
    LOG_DBG("get_identity_cb");
    if (NULL != identity) {
        *identity = &mender_identity;
        return MENDER_OK;
    }
    return MENDER_FAIL;
}

static void set_leds(const struct led_rgb data[]) {
	int rc = led_strip_update_rgb(strip, (struct led_rgb *)data, STRIP_NUM_PIXELS);
    if (rc) {
        LOG_ERR("couldn't update strip: %d", rc);
    }
}


static mender_err_t
persistent_inventory_cb(mender_keystore_t **keystore, uint8_t *keystore_len) {
    static mender_keystore_t inventory[] = { { .name = "App", .value = "mender-mcu-integration" } };
    *keystore                            = inventory;
    *keystore_len                        = 1;
    return MENDER_OK;
}

int
main(void) {
    int led_ready = 0;
    int toggle = 0;

    uint8_t chip_id[16];
    char id_str[sizeof(chip_id) * 2 + 1];
    ssize_t length = hwinfo_get_device_id(chip_id, sizeof(chip_id));

    if (length > 0) {
        bin2hex(chip_id, length, mender_identity.value, sizeof(id_str));

        LOG_INF("ESP32 Chip ID: %s", mender_identity.value);
    } else {
        LOG_ERR("Failed to get Chip ID");
    }

    if (device_is_ready(strip)) {
		LOG_INF("Found LED strip device %s", strip->name);
        led_ready = 1;
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		goto END;
    }

    if (led_ready) {
		LOG_INF("Clearing LEDS...");
        set_leds(pixels_off);
		LOG_INF(".. setting startup LEDS.");
        set_leds(pixels_boot);
    }

    netup_wait_for_network();

    certs_add_credentials();

    /* Initialize mender-client */
    mender_client_config_t    mender_client_config    = { .device_type = CONFIG_MENDER_DEVICE_TYPE, .recommissioning = false };
    mender_client_callbacks_t mender_client_callbacks = { .network_connect        = mender_network_connect_cb,
                                                          .network_release        = mender_network_release_cb,
                                                          .deployment_status      = mender_deployment_status_cb,
                                                          .restart                = mender_restart_cb,
                                                          .get_identity           = mender_get_identity_cb,
                                                          .get_user_provided_keys = NULL };

    LOG_INF("Initializing Mender Client with:");
    LOG_INF("   Device type:   '%s'", mender_client_config.device_type);
    LOG_INF("   Identity:      '{\"%s\": \"%s\"}'", mender_identity.name, mender_identity.value);

    if (MENDER_OK != mender_client_init(&mender_client_config, &mender_client_callbacks)) {
        LOG_ERR("Failed to initialize the client");
        goto END;
    }
    LOG_INF("Mender client initialized");

#ifdef CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE
    if (MENDER_OK != mender_zephyr_image_register_update_module()) {
        LOG_ERR("Failed to register the zephyr-image Update Module");
        goto END;
    }
    LOG_INF("Update Module 'zephyr-image' initialized");
#endif /* CONFIG_MENDER_ZEPHYR_IMAGE_UPDATE_MODULE */

#ifdef CONFIG_MENDER_APP_NOOP_UPDATE_MODULE
    if (MENDER_OK != noop_update_module_register()) {
        LOG_ERR("Failed to register the noop Update Module");
        goto END;
    }
    LOG_INF("Update Module 'noop-update' initialized");
#endif /* CONFIG_MENDER_APP_NOOP_UPDATE_MODULE */

#ifdef BUILD_INTEGRATION_TESTS
    if (MENDER_OK != test_update_module_register()) {
        LOG_ERR("Failed to register the test Update Module");
        goto END;
    }
    LOG_INF("Update Module 'test-update' initialized");
#endif /* BUILD_INTEGRATION_TESTS */

    if (MENDER_OK != mender_inventory_add_callback(persistent_inventory_cb, true)) {
        LOG_ERR("Failed to add inventory callback");
        goto END;
    }
    LOG_INF("Mender inventory callback added");

    /* Finally activate mender client */
    if (MENDER_OK != mender_client_activate()) {
        LOG_ERR("Unable to activate the client");
        goto END;
    }
    LOG_INF("Mender client activated and running!");

    while (1) {
        if (toggle) {
            toggle = 0;
            set_leds(pixels_payload1);
        }
        else {
            toggle = 1;
            set_leds(pixels_payload2);
        }
        //LOG_INF("ESP32 CHIP ID: %s", mender_identity.value);
		k_msleep(SLEEP_TIME_MS);
    }

END:
    k_sleep(K_FOREVER);

    return 0;
}
