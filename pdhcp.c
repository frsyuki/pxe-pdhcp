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

#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include "pdhcp.h"
#include "dhcp.h"


int check_dhcp_packet(struct dhcp_packet *p)
{
	u_char *op;

	/* check magic cookie */
	if (memcmp(p->options, DHCP_OPTIONS_COOKIE, DHCP_OPTIONS_COOKIE_LEN))
		return -1;

	/* */
	if (p->op != BOOTREQUEST)
		return -1;

	/* check packet length */
	op = p->options + DHCP_OPTIONS_COOKIE_LEN;
	while (*op != DHO_END) {
		if (op - p->options >= DHCP_OPTION_LEN)
			return -1;
		else if (*op == DHO_PAD)
			++op;
		else 
			op += op[1] + 2;
	}
	return 0;
}

int get_dhcp_option(struct dhcp_packet *p, u_char type, u_char **opt, u_char *optlen)
{
	u_char *opos = p->options + DHCP_OPTIONS_COOKIE_LEN;
	while (*opos != DHO_END) {
		if (*opos == type) {
			*opt = &opos[2];
			*optlen = opos[1];
			return 0;
		}
		if (*opos == DHO_PAD)
			opos++;
		else
			opos += opos[1] + 2;
	}
	return -1;
}

int get_dhcp_message_type(struct dhcp_packet *p)
{
	u_char *opt, optlen;
	if (get_dhcp_option(p, DHO_DHCP_MESSAGE_TYPE, &opt, &optlen) == -1) {
		return -1;
	}
	return *opt;
}

int get_dhcp_packet_len(struct dhcp_packet *p)
{
	u_char *op = p->options;
	op += DHCP_OPTIONS_COOKIE_LEN;
	while (*op != DHO_END) {
		if (*op == DHO_PAD)
			op++;
		else
			op += op[1] + 2;
	}
	return (op - (u_char*)p) + 1;
}

int add_dhcp_option(struct dhcp_packet *p, u_char type,
		u_char *opt, u_char optlen)
{
	u_char *op;

	int len = get_dhcp_packet_len(p);
	if (len == -1) return -1;

	/* check if there is enough space left */
	if (len + optlen + 3 > sizeof(struct dhcp_packet))
		return -1;

	/* add new option */
	op = (u_char*)p + len - 1;
	op[0] = type;
	op[1] = optlen;
	memcpy(&op[2], opt, optlen);

	/* and end marker */
	op[2+optlen] = DHO_END;

	return 0;
}

