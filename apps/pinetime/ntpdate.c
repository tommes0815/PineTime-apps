#include <stdio.h>
#include <time.h>

#include "net/sntp.h"
#include "net/ntp_packet.h"
#include "net/af.h"
#include "net/ipv6/addr.h"
#include "timex.h"
#include "controller.h"

// fe80::1
#define NTPDATE_SERVADDR { \
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}

#define DEFAULT_TIMEOUT 5000000
static int time_set = 0;

#define TIMEZONE_OFFSET (120) /* Minutes */


int ntpdate(void)
{
    uint32_t timeout = DEFAULT_TIMEOUT;
    struct tm tm;
    time_t time;
    controller_t *ctl;
    if (time_set)
        return 0;
    controller_time_spec_t ts;
    gnrc_netif_t *netif = NULL;

    netif = (gnrc_netif_t *)netif_iter(NULL);
    if (!netif)
        return 1;

    sock_udp_ep_t server = { .port = NTP_PORT, .family = AF_INET6, .addr.ipv6 = NTPDATE_SERVADDR};
    server.netif = netif->pid;
    if (sntp_sync(&server, timeout) < 0) {
        puts("Error in synchronization");
        return 1;
    }
    time = (time_t)(sntp_get_unix_usec() / US_PER_SEC);
    gmtime_r(&time, &tm);

    ts.second = tm.tm_sec;
    ts.minute = tm.tm_min + TIMEZONE_OFFSET;
    ts.hour = tm.tm_hour;
    ts.dayofmonth = tm.tm_mday;
    ts.year = tm.tm_year + 1900;
    ts.month = tm.tm_mon + 1;
    ts.fracs = 0;
    ctl = controller_get();
    controller_time_set_time(ctl, &ts);
    controller_update_time(ctl);
    time_set = 1;
    return 0;
}
