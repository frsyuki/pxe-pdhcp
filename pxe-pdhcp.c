/*
 * pxe-pdhcp
 *
 * Copyright (c) 2007 FURUHASHI Sadayuki <fr _at_ syuki.skr.jp>
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

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "pdhcp.h"
#include "dhcp.h"


#ifndef NDEBUG
#define DBG(msg, args...) { char _ts[20] = {0}; time_t _t; struct tm _tm; \
                            time(&_t); localtime_r(&_t, &_tm);            \
                            strftime(_ts, 19, "%b %d %H:%M:%S", &_tm);    \
                            printf("%s: DEBUG: " __FILE__ ": ", _ts);     \
                            printf(msg, ## args); printf("\n"); }
#else
#define DBG(msg, args...) { }
#endif

#define LOG(msg, args...) { char _ts[20] = {0}; time_t _t; struct tm _tm; \
                            time(&_t); localtime_r(&_t, &_tm);            \
                            strftime(_ts, 19, "%b %d %H:%M:%S", &_tm);    \
                            printf("%s: " __FILE__ ": ", _ts);            \
                            printf(msg, ## args); printf("\n"); }

int stopflag = 0;
int foreground = 0;

struct sockaddr_in server_address;
struct sockaddr_in bcast_address;
struct in_addr tftp_ip;
char* nbp_name;

int server_socket;


void init_packet(struct dhcp_packet *newp, struct dhcp_packet *oldp, u_char mtype)
{
	u_char *op;

	memset(newp, 0, sizeof(*newp));
	newp->op = BOOTREPLY;
	newp->htype = oldp->htype;
	newp->hlen  = oldp->hlen;
	newp->xid   = oldp->xid;
	newp->flags = oldp->flags;

	/* set DHCP Cookie */
	memcpy(newp->options, DHCP_OPTIONS_COOKIE, DHCP_OPTIONS_COOKIE_LEN);
	op = &newp->options[DHCP_OPTIONS_COOKIE_LEN];

	/* set DHCP Message Type */
	op[0] = DHO_DHCP_MESSAGE_TYPE;
	op[1] = 1;
	op[2] = mtype;
	op += 3;

	/* set Server Identifier */
	op[0] = DHO_DHCP_SERVER_IDENTIFIER;
	op[1] = sizeof(struct in_addr);
	memcpy(&op[2], &server_address.sin_addr, sizeof(struct in_addr));
	op += (2 + sizeof(struct in_addr));

	*op = DHO_END;
}

void dhcp_reply(struct dhcp_packet *p, struct sockaddr* client_address, socklen_t client_addrlen)
{
	int packet_len;
	u_char *opt;
	u_char optlen;
	struct dhcp_packet re;
	int res;

	DBG("dhcp_reply start");

	if (check_dhcp_packet(p) == -1) {
		DBG("got a invalid dhcp packet");
		return;
	}
	if (get_dhcp_message_type(p) == DHCPDISCOVER) {
		DBG("got a DHCPDISCOVER on dhcp");
	} else {
		DBG("got a non-DHCPDISCOVER dhcp packet");
		return;
	}

	/* check vendor class identifier ("PXEClient") */
	res = get_dhcp_option(p, DHO_VENDOR_CLASS_IDENTIFIER, &opt, &optlen);
	if (res == -1 || strncmp((char*)opt, PXE_IDENTIFIER_STRING, strlen(PXE_IDENTIFIER_STRING)) != 0) {
		LOG("got a non-PXE dhcp packet");
		return;
	}

	/* create response */
	init_packet(&re, p, DHCPOFFER);
	/* re.ciaddr is 0.0.0.0 */
	/* re.yiaddr is 0.0.0.0 */
	re.siaddr = tftp_ip;
	memcpy(re.chaddr, p->chaddr, sizeof(re.chaddr));
	strcpy(re.file, nbp_name);

	/* set vendor class identifier */
	res = add_dhcp_option(&re, DHO_VENDOR_CLASS_IDENTIFIER,
		(u_char*)PXE_IDENTIFIER_STRING,
		strlen(PXE_IDENTIFIER_STRING)+1);
	if (res == -1) {
		DBG("add_dhcp_option() failed");
		return;
	}

	/* set vendor options */
	{
		u_char vop[3];
		vop[0] = PXE_DISCOVERY_CONTROL;
		vop[1] = 1;
		vop[2] = PXE_DISCOVERY_CONTROL_BIT3;
		if (add_dhcp_option(&re, DHO_VENDOR_ENCAPSULATED_OPTIONS, vop, 3) == -1) {
			DBG("add_dhcp_option() failed");
			return;
		}
	}

	/* send the response */
	packet_len = get_dhcp_packet_len(&re);
	res = sendto(server_socket, &re, packet_len, 0,
		(struct sockaddr*)&bcast_address, sizeof(bcast_address));
	if (res != packet_len) {
		LOG("sendto() failed or was not complete: %s", strerror(errno));
		return;
	}

	DBG("dhcp_reply end");
}

void pdhcp(void)
{
	struct dhcp_packet dhcp_p;
	struct sockaddr_in client_address;
	socklen_t client_addrlen;
	int res;
	while(!stopflag) {
		memset(&dhcp_p, 0, sizeof(dhcp_p));
		client_addrlen = sizeof(client_address);
		res = recvfrom(server_socket, &dhcp_p, sizeof(dhcp_p), 0,
			(struct sockaddr*)&client_address, &client_addrlen);
		if (res == -1) {
			LOG("recvfrom() failed: %s", strerror(errno));
			continue;
		} 
		dhcp_reply(&dhcp_p, (struct sockaddr*)&client_address,
			client_addrlen);
	}
}

int init_socket(void)
{
	int res;
	int on;

	server_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (server_socket == -1) return -1;

	on = 1;
	res = setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	if (res == -1) return -1;

	res = bind(server_socket, (struct sockaddr*)&server_address,
			sizeof(server_address));
	if (res == -1) return -1;

	return 0;
}

void release_all(void)
{
	close(server_socket);
}

void signal_handler(int sig)
{
	if (sig == SIGTERM) stopflag = 1;
	if (sig == SIGQUIT) stopflag = 1;
	if (sig == SIGINT) stopflag = 1;
	DBG("got signal %d", sig);
}
int install_signal_handler(void)
{
	struct sigaction action;

	/* set SIGTERM handler */
	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGTERM, &action, NULL) == -1)
		return -1;

	/* set SIGQUIT handler */
	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGQUIT, &action, NULL) == -1)
		return -1;

	/* set SIGINT handler */
	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGINT, &action, NULL) == -1)
		return -1;

	return 0;
}

void usage(const char* prog)
{
	fprintf(stderr, "usage: %s  [-d] [-i interface] [-l listen address] [-t tftp address] [-b broadcast address] <nbp name>\n", prog);
	fprintf(stderr, "-i or (-l, -t and -b), and <nbp name> options are needed.\n");
}

void parse_argv(int argc, char* argv[])
{
	int opt;

	char* listen_dev;
	int set_server_address = 0;
	int set_bcast_address = 0;
	int set_tftp_ip  = 0;

	memset(&server_address, 0, sizeof(server_address));
	memset(&bcast_address, 0, sizeof(bcast_address));
	memset(&tftp_ip,  0, sizeof(tftp_ip));
	listen_dev = NULL;
	nbp_name = NULL;


	while ((opt = getopt(argc, argv, "i:l:b:t:d")) != -1) {
		switch(opt) {
		case 'i':
			listen_dev = optarg;
			break;
		case 'l':
			if (!inet_aton(optarg, &server_address.sin_addr)) {
				perror(optarg);
				exit(1);
			}
			set_server_address = 1;
			break;
		case 'b':
			if (!inet_aton(optarg, &bcast_address.sin_addr)) {
				perror(optarg);
				exit(1);
			}
			set_bcast_address = 1;
			break;
		case 't':
			if (!inet_aton(optarg, &tftp_ip)) {
				perror(optarg);
				exit(1);
			}
			set_tftp_ip = 1;
			break;
		case 'd':
			foreground = 1;
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}
	if (optind >= argc) {
		usage(argv[0]);
		exit(1);
	}
	nbp_name = argv[optind];

	if (!set_server_address || !set_tftp_ip || !set_bcast_address) {
		/* get these addresses from listen_dev */
		struct ifreq ifr;
		int s;

		if (!listen_dev) {
			usage(argv[0]);
			exit(1);
		}

		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s == -1) {
			perror("socket() failed");
			exit(1);
		}
		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, listen_dev, IFNAMSIZ-1);
		ifr.ifr_name[IFNAMSIZ] = '\0';

		if (!set_server_address || !set_tftp_ip) {
			if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
				perror("cannot get ip address of listen interface");
				close(s);
				exit(1);
			}
			if (!set_server_address)
				server_address.sin_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
			if (!set_tftp_ip)
				tftp_ip = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
		}

		if (!set_bcast_address) {
			if (ioctl(s, SIOCGIFBRDADDR, &ifr) == -1) {
				perror("cannot get broadcast address of listen interface");
				close(s);
				exit(1);
			}
			bcast_address.sin_addr =
				((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr;
		}

		close(s);
	}

	server_address.sin_port = ntohs(DHCP_SERVER_PORT);
	server_address.sin_family = AF_INET;
	bcast_address.sin_port = ntohs(DHCP_CLIENT_PORT);
	bcast_address.sin_family = AF_INET;
}

int main(int argc, char* argv[])
{
	parse_argv(argc, argv);

	printf("listen address:     %s:%d\n", inet_ntoa(server_address.sin_addr),
		ntohs(server_address.sin_port));
	printf("broadcast address   %s:%d\n", inet_ntoa(bcast_address.sin_addr),
		ntohs(bcast_address.sin_port));
	printf("tftp address:       %s\n", inet_ntoa(tftp_ip));
	printf("nbp name:           %s\n", nbp_name);

	/* install signal handler */
	if (install_signal_handler() == -1) {
		perror("sigaction() failed");
		exit(-1);
	}

	/* init sockets */
	if (init_socket() == -1) {
		perror("init_socket()");
		release_all();
		exit(1);
	}

	if (foreground) {
		pdhcp();
	} else {
		daemon(0,0);
		pdhcp();
	}

	return 0;
}

