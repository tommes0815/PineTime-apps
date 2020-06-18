/* fwupdate.c
 *
 * Copyright (C) 2020 Daniele Lacamera
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *=============================================================================
 *
 *
 */

#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in6 */
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FWUP_PORT 7777
#define PAGE_SIZE 256
#define DGRAM_SIZE (PAGE_SIZE + 4)


static struct sockaddr_in6 servAddr;        /* our server's address */

void send_dgram(int fd, int sd, uint32_t pos)
{
    unsigned char datagram[DGRAM_SIZE];
    uint32_t *seqn = (uint32_t *)datagram;
    uint8_t *payload = (uint8_t *)(datagram + 4);
    int ret;
    *seqn = pos;
    lseek(fd, SEEK_SET, pos);
    ret = read(fd, payload, PAGE_SIZE);
    if (ret <= 0)
        return;
    sendto(sd, datagram, ret + 4, 0, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in6));
    printf("Sent: %u\n", pos);
}

int main(int argc, char** argv)
{
    uint32_t tlen, ack_seq;
    int ffd, res, sfd;
    uint32_t ack;
    struct stat st;
    if (argc != 3) {
        printf("Usage: %s firmware_filename IPaddress\n", argv[0]);
        exit(1);
    }

    ffd = open(argv[1], O_RDONLY);
    if (ffd < 0) {
        perror("opening file");
        exit(2);
    }

    res = fstat(ffd, &st);
    if (res != 0) {
        perror("fstat file");
        exit(2);
    }
    servAddr.sin6_family = AF_INET6;
    servAddr.sin6_port = htons(FWUP_PORT);

    sfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("creating socket");
        exit(2);
    }

    if (inet_pton(AF_INET6, argv[2], &servAddr.sin6_addr) != 1) {
        perror("invalid IP address");
        exit(2);
    }
    tlen = st.st_size;
    ack_seq = 0;

    while (1) {
        if (ack_seq == 0) {
            printf("Starting new firmware upload...");
            send_dgram(ffd, sfd, 0);
            sleep(1);
        }
        res = recv(sfd, &ack, 4, 0);
        if (ack == (uint32_t)(-1)) {
            printf("Device reported an error. Bailing out.\n");
            exit(1);
        }
        if (ack != ack_seq)
            ack_seq = ack;
        send_dgram(ffd, sfd, ack_seq);

        if (tlen <= (ack + PAGE_SIZE)) {
            printf("Waiting for last ack...\n");
            res = recv(sfd, &ack, 4, 0);
            if ( (res == 4) || ack == tlen) {
                printf("Upload successful!\n");
                return 0;
            }
        }
    }
    return 0;
}
