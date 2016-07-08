pxe-pdhcp
===========

A implementation of Preboot Execution Environment (PXE) server.

pxe-pdhcp works with DHCP server running on another host. The DHCP server
doesn't need to be configured for any PXE specific options. This means that
you can set up network boot environment without re-configuring existent DHCP
server.

    usage: pxe-pdhcp  [-d] [-i interface]
                      [-l listen address] [-t tftp address] [-b broadcast address]
                      <nbp name>

pxe-pdhcp listens on two well-known ports (67/udp and 68/udp) so the root
privilege is needed to run.

With -d option, pxe-pdhcp runs on foreground. This option is useful for debugging.

Specify nework interface name ('eth0' for example, on Linux) to -i option.

If the network interface has more than one IP address, use -l, -t, -b options.
-i or (-l,-t and -b) option is required.

Finally specify the path of Network Bootstrap Program on the TFTP server to
<nbp name> option. It will be 'pxelinux.0' if you are using PXELINUX.

----

    Copyright (c) 2007-2016 Sadayuki Furuhashi
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    
    dhcp.h is provided by ISC-DHCP copyright by Internet Systems Consortium
    licensed under MIT license. the original software and related information is
    available at https://www.isc.org/software/dhcp/.

