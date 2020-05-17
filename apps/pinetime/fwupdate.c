#include <stdint.h>
#include "assert.h"
#include "event/timeout.h"
#include "periph/pm.h"
#include "net/sock.h"
#include "net/sock/udp.h"
#include "wolfboot/wolfboot.h"
#include "hal.h"
#include "mtd.h"
#include "gatt_server.h"
#define FWUP_STACKSIZE (1024)
#define FWUP_PRIO 4
#define FWUP_PORT 7777
#define PAGESIZE 256
#define SOCK_RECV_TIMEOUT 2000000

#define RESERVED_AREA (8192) // Image must not write into the last two sectors

#define DGRAM_SIZE (PAGESIZE + sizeof(uint32_t))
static uint8_t fwup_stack[FWUP_STACKSIZE];
static uint8_t page[DGRAM_SIZE];
static uint8_t *payload = (page + sizeof(uint32_t));
static uint32_t *seqn = (uint32_t *)page;

static int pan_connected = 0;
int ntpdate(void);


void *fwup_thread(void *arg)
{
    int r;
    uint32_t tlen = 0;
    volatile uint32_t recv_seq, sz;
    sock_udp_t sk;
    sock_udp_ep_t local = {};
    sock_udp_ep_t remote;
    local.family = AF_INET6;
    local.port = FWUP_PORT;
    if (sock_udp_create(&sk, &local, NULL, 0) < 0) {
        return NULL;
    } 
    memset(page, 0xFF, DGRAM_SIZE);
    while (1) {
        if (!pan_connected) {
            if (gatt_pan_connected()) {
                pan_connected = 1;
                ntpdate();
            }
            xtimer_sleep(2);
            continue;
        }
        if (!gatt_pan_connected()) {
            pan_connected = 0;
            continue;
        }
        if (tlen == 0) {
            if (gatt_pan_connected()) {
                ntpdate();
            }
            r = sock_udp_recv(&sk, page, DGRAM_SIZE, SOCK_RECV_TIMEOUT, &remote);
            if (r != DGRAM_SIZE)
                continue;
            if (*((uint32_t *)page) != 0)
                continue;
            if (strncmp(payload, "WOLF", 4) != 0)
                continue;
            /* Start update */
            tlen = (*(uint32_t *)(payload + 4)) + 256; /* 256 == manifest header size */
            if (tlen > WOLFBOOT_PARTITION_SIZE - RESERVED_AREA)
                continue;
            mtd_erase(mtd0, WOLFBOOT_PARTITION_UPDATE_ADDRESS, WOLFBOOT_PARTITION_SIZE);
            mtd_write(mtd0, payload, WOLFBOOT_PARTITION_UPDATE_ADDRESS, PAGESIZE);
            recv_seq = PAGESIZE;
        }
        /* Send ACK with expected seq */
        sock_udp_send(&sk, (void *)&recv_seq, 4, &remote);

        /* Calculate next datagram size */
        sz = PAGESIZE;
        if ((tlen - recv_seq) < PAGESIZE)
            sz = tlen - recv_seq;
        r = sock_udp_recv(&sk, page, DGRAM_SIZE, SOCK_RECV_TIMEOUT, NULL);
        if (r < (sz + sizeof(uint32_t)))
            continue;
        if (*seqn != recv_seq)
            continue;

        mtd_write(mtd0, payload, WOLFBOOT_PARTITION_UPDATE_ADDRESS + recv_seq, sz);
        recv_seq += sz;

        if (recv_seq >= tlen) {
            /* Update complete */
            sock_udp_send(&sk, (void *)&tlen, 4, &remote);
            //wolfBoot_update_trigger();
            mtd_write(mtd0, "pBOOT", WOLFBOOT_PARTITION_UPDATE_ADDRESS + WOLFBOOT_PARTITION_SIZE - 5, 5);
            tlen = 0;
            recv_seq = 0;
            xtimer_sleep(1);
            continue;
        }
    }
    return NULL;
}

void fwupdate_thread_create(void)
{
    int res = thread_create(fwup_stack, FWUP_STACKSIZE, FWUP_PRIO,
                            THREAD_CREATE_STACKTEST, fwup_thread,
                            NULL, "fwup");
    wolfBoot_success();
}
