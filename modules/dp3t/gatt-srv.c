/*
 * Copyright (C) 2020 Dyne.org foundation
 *
 * This file is subject to the terms and conditions of the Affero GNU
 * General Public License (AGPL) version 3. See the file LICENSE for
 * more details.
 *
 */

#include <stdio.h>
#include <stdint.h>

#include "assert.h"
#include "event/timeout.h"
#include "nimble_riot.h"
#include "net/bluetil/ad.h"
#include "nimble_netif_conn.h"

#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "dp3t.h"
#include "sys_random.h"
#include "keystore.h"

#define HRS_FLAGS_DEFAULT       (0x01)      /* 16-bit BPM value */
#define SENSOR_LOCATION         (0x02)      /* wrist sensor */
#define UPDATE_INTERVAL         (250U * US_PER_MS)

static const char *device_name = "DP3T";
static const char *_manufacturer_name = "Dyne.org";
static const char *_model_number = "1";

static event_queue_t _eq;
static event_t _update_evt;

static uint16_t _conn_handle;
static uint16_t _hrs_val_handle;

static int pan_connected = 0;
static int pan_advertising = 0;

int gatt_pan_advertising(void)
{
    return pan_advertising;
}

int gatt_pan_connected(void)
{
    return pan_connected;
}

static int _devinfo_handler(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg);

static void pan_event_handler(int handle, nimble_netif_event_t ev, const uint8_t *addr)
{
    switch (ev) {
        case NIMBLE_NETIF_CONNECTED_MASTER:/**< connection established as master */
        case NIMBLE_NETIF_CONNECTED_SLAVE:   /**< connection established as slave */
            pan_connected = 1;
            break;

        case NIMBLE_NETIF_CLOSED_MASTER:     /**< connection closed (we were master) */
        case NIMBLE_NETIF_CLOSED_SLAVE:      /**< connection closed (we were slave) */
        case NIMBLE_NETIF_ABORT_MASTER:      /**< connection est. abort (as master) */
        case NIMBLE_NETIF_ABORT_SLAVE:       /**< connection est. abort (as slave) */
            pan_connected = 0;
            break;
        default:
            break;

    }
}

void gatt_pan_start(const char *name)
{
    int res;
    (void)res;
    uint8_t buf[BLE_HS_ADV_MAX_SZ];
    bluetil_ad_t ad;
    const struct ble_gap_adv_params _adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_LTD,
        .itvl_min = BLE_GAP_ADV_FAST_INTERVAL2_MIN,
        .itvl_max = BLE_GAP_ADV_FAST_INTERVAL2_MAX,
    };
    
    if (name == NULL)
        return;

    /* make sure no advertising is in progress */
    if (nimble_netif_conn_is_adv()) {
        puts("err: advertising already in progress");
        return;
    }

    /* build advertising data */
    res = bluetil_ad_init_with_flags(&ad, buf, BLE_HS_ADV_MAX_SZ,
                                     BLUETIL_AD_FLAGS_DEFAULT);
    assert(res == BLUETIL_AD_OK);
    uint16_t ipss = BLE_GATT_SVC_IPSS;
    res = bluetil_ad_add(&ad, BLE_GAP_AD_UUID16_INCOMP, &ipss, sizeof(ipss));
    assert(res == BLUETIL_AD_OK);
    res = bluetil_ad_add(&ad, BLE_GAP_AD_NAME, name, strlen(name));
    if (res != BLUETIL_AD_OK) {
        puts("err: the given name is too long");
        return;
    }

    /* Set notifications callback */
    nimble_netif_eventcb(pan_event_handler);

    /* start listening for incoming connections */
    res = nimble_netif_accept(ad.buf, ad.pos, &_adv_params);
    if (res != NIMBLE_NETIF_OK) {
        printf("err: unable to start advertising (%i)\n", res);
    }
    else {
        printf("success: advertising this node as '%s'\n", name);
    }
    pan_advertising = 1;
}

void gatt_pan_stop(void)
{
    int res = nimble_netif_accept_stop();
    if (res == NIMBLE_NETIF_OK) {
        puts("canceled advertising");
    }
    else if (res == NIMBLE_NETIF_NOTADV) {
        puts("no advertising in progress");
    }
    pan_advertising = 0;
}


/* GATT service definitions */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Device Information Service */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_GATT_SVC_DEVINFO),
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_MANUFACTURER_NAME),
            .access_cb = _devinfo_handler,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_MODEL_NUMBER_STR),
            .access_cb = _devinfo_handler,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_SERIAL_NUMBER_STR),
            .access_cb = _devinfo_handler,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_FW_REV_STR),
            .access_cb = _devinfo_handler,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            .uuid = BLE_UUID16_DECLARE(BLE_GATT_CHAR_HW_REV_STR),
            .access_cb = _devinfo_handler,
            .flags = BLE_GATT_CHR_F_READ,
        }, {
            0, /* no more characteristics in this service */
        }, }
    },
    {
        0, /* no more services */
    },
};

static int _devinfo_handler(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    const char *str;

    switch (ble_uuid_u16(ctxt->chr->uuid)) {
        case BLE_GATT_CHAR_MANUFACTURER_NAME:
            //printf("[READ] device information service: manufacturer name value");
            str = _manufacturer_name;
            break;
        case BLE_GATT_CHAR_MODEL_NUMBER_STR:
            //printf("[READ] device information service: model number value");
            str = _model_number;
            break;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }

    int res = os_mbuf_append(ctxt->om, str, strlen(str));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    /* Used only if we want to export other GATT services */
    return 0;
}

static void gatt_dp3t_start_advertisements(void)
{
    struct ble_gap_adv_params advp;
    int res;

    memset(&advp, 0, sizeof advp);
    advp.conn_mode = BLE_GAP_CONN_MODE_UND;
    advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
    advp.itvl_min  = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    advp.itvl_max  = BLE_GAP_ADV_FAST_INTERVAL1_MAX;
    res = ble_gap_adv_start(nimble_riot_own_addr_type, NULL, BLE_HS_FOREVER,
                            &advp, gap_event_cb, NULL);
    assert(res == 0);
    (void)res;
}

int gatt_dp3t_server(void)
{
    int res = 0;
    uint8_t buf[31];
    uint16_t hdr_txpow = BLE_GAP_AD_TX_POWER_LEVEL;
    uint16_t hdr_uuid = BLE_GAP_AD_UUID128_COMP;
    uint8_t tx_pow = 0x00;
    uint8_t addr[6];

    //printf("BLE DP3T service started");
    /* verify and add our custom services */
    res = ble_gatts_count_cfg(gatt_svr_svcs);
    assert(res == 0);
    res = ble_gatts_add_svcs(gatt_svr_svcs);
    assert(res == 0);

    /* set the device name */
    ble_svc_gap_device_name_set("");

    /* reload the GATT server to link our added services */
    ble_gatts_start();

    //sys_random(addr, 6);
    //ble_hs_id_set_rnd(addr);

    /* configure and set the advertising data */
    bluetil_ad_t ad;
    bluetil_ad_init_with_flags(&ad, buf, sizeof(buf), BLE_GAP_DISCOVERABLE);
    bluetil_ad_add(&ad, hdr_txpow, &tx_pow, 1);
    bluetil_ad_add(&ad, hdr_uuid, dp3t_get_ephid(0), EPHID_LEN);
    ble_gap_adv_set_data(ad.buf, ad.pos);

    /* start to advertise this node */
    gatt_dp3t_start_advertisements();
    return 0;
}
