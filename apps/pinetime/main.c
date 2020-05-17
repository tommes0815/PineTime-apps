/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "log.h"
#include "xtimer.h"
#include "lvgl.h"
#include "controller.h"
#include "hal/hal.h"
#include "shell.h"
#include "msg.h"
#include "dp3t.h"
#include "keystore.h"
#include "ble_scan.h"
#include "gatt_server.h"
#include "net/af.h"
#include "net/netif.h"
#include "net/ipv6/addr.h"
#include "timex.h"

static void bzz(uint32_t us)
{
    gpio_clear(VIBRATOR);
    xtimer_usleep(us);
    gpio_set(VIBRATOR);
}

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int lvgl_thread_create(void);
void fwupdate_thread_create(void);

void ip_add_sitelocal(netif_t *iface)
{
    int flags = (64<< 8U);
    ipv6_addr_t sitelocal_addr = { .u8 = 
    { 0xFd,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02} };

    if (netif_set_opt(iface, NETOPT_IPV6_ADDR, flags, &sitelocal_addr, sizeof(sitelocal_addr)) < 0) {
        printf("error: unable to add IPv6 address\n");
        return;
    }
}


int main(void)
{
    int pan_connected = 0;
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    LOG_DEBUG("Starting PineTime application");
    lv_init();

    hal_init();
    lvgl_thread_create();

    /* MTD for SPI flash access */
    spi_init(PINETIME_NOR_SPI_DEV);
    mtd_init(mtd0);

    /* UDP firmware update */
    fwupdate_thread_create();

    /* dp3t */
    dp3t_start();

    /* Start dp3t gatt advertisements */
    //gatt_server();
    // OR
    /* Start PAN */
    gatt_pan_start("watch");

    /* Start dp3t scan service  */
    dp3t_blescan_init();
    controller_thread_create();

    /* bzz */
    bzz(100000);
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);


#if 0
    while(1) {
        if (gatt_pan_connected() != 0) {
            if (!pan_connected) {
                pan_connected = 1;
                ntpdate();
            }
        } else {
            pan_connected = 0;
        }
        ntpdate();
        xtimer_sleep(20);
    }
#endif
    return 0;
}
