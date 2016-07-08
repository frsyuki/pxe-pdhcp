/*
 * pxe-pdhcp
 *
 * Copyright (c) 2007-2016 Sadayuki Furuhashi
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef PXE_PDHCP_H
#define PXE_PDHCP_H 1

#include <sys/types.h>
#include <arpa/inet.h>
#include "dhcp.h"

#define DHCP_SERVER_PORT   67
#define DHCP_CLIENT_PORT   68

#define DHCP_OPTIONS_COOKIE_LEN  4

#define PXE_IDENTIFIER_STRING           "PXEClient"

#define PXE_MTFTP_IP                    1
#define PXE_MTFTP_CPORT                 2
#define PXE_MTFTP_SPORT                 3
#define PXE_MTFTP_TMOUT                 4
#define PXE_MTFTP_DELAY                 5
#define PXE_DISCOVERY_CONTROL           6
#define PXE_DISCOVERY_MCAST_ADDR        7
#define PXE_BOOT_SERVERS                8
#define PXE_BOOT_MENU                   9
#define PXE_MENU_PROMPT                 10
#define PXE_MCAST_ADDRS_ALLOC           11
#define PXE_CREDENTIAL_TYPES            12
#define PXE_END                         255

#define PXE_DISCOVERY_CONTROL_BIT0      0x01
#define PXE_DISCOVERY_CONTROL_BIT1      0x02
#define PXE_DISCOVERY_CONTROL_BIT2      0x04
#define PXE_DISCOVERY_CONTROL_BIT3      0x08

int check_dhcp_packet(struct dhcp_packet *p);
int get_dhcp_option(struct dhcp_packet *p, u_char type,
		u_char **opt, u_char *optlen);
int get_dhcp_message_type(struct dhcp_packet *p);
int get_dhcp_packet_len(struct dhcp_packet *p);
int add_dhcp_option(struct dhcp_packet *p, u_char type,
		u_char *opt, u_char optlen);

#endif

