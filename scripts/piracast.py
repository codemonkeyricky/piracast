#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# This file is part of Piracast.
#
#     Piracast is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     Piracast is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with Piracast.  If not, see <http://www.gnu.org/licenses/>.

import os
import re
import time

import wfd
import core_channel
import sink

from util import get_stdout

lease_file = './dhcpd.leases'

cmd_wlan0_up = 'ifup wlan0'
cmd_inc_rmem_default = 'sysctl -w net.core.rmem_default=1000000'
cmd_launch_core_app = 'nice -n -20 ./piracast_core 1>/dev/null 2>&1 &'
#cmd_kill_core_app = 'python core_terminate.py'
cmd_kill_core_app = 'pkill piracast_core'
cmd_dhcp_start = ['/usr/sbin/dhcpd', '-pf', './dhcpd.pid', '-cf', '../env/dhcpd.conf', '-lf', lease_file, 'wlan0']
cmd_dhcp_stop = 'kill $(cat ./dhcpd.pid)'

def lease_file_timestamp_get():
    return get_stdout('ls -l "%s"' % lease_file)

# get the leased IP address
def leased_ip_get():
    contents = open(lease_file).read()
    ip_list = re.findall(r'lease (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', contents)

    # return the most recently leased IP address
    return ip_list[-1]

print 'Bring up wlan0 just in case...'
get_stdout(cmd_wlan0_up)

print 'Increase rmem_default...'
get_stdout(cmd_inc_rmem_default)

print 'Kill running application...'
#core_channel.end()
print get_stdout(cmd_kill_core_app)

while 1:

    # Launch application
    get_stdout(cmd_launch_core_app)

    # Start DHCP
    print get_stdout(cmd_dhcp_start)

    # Get previous timestamp
    prev_ts = lease_file_timestamp_get()

    # Wait for connection
    wfd.wfd_connection_wait()

    # Wait until lease file is updated
    while 1:

        curr_ts = lease_file_timestamp_get()

        if curr_ts != prev_ts:

            print 'Source has requested IP!'

            # wait for network to be properly configured
            time.sleep(2)

            break

        print 'lease table has not been updated, wait for a second...'

        time.sleep(1)

    # Get source IP
    ip = leased_ip_get()
    print 'leased IP: ', ip

    port = wfd.p2p_peer_port_get()
    print 'peer port: ', port

    # Connect to source
    sink.source_connect(ip, port)

    # Stop DHCPd
    output = get_stdout(cmd_dhcp_stop)
    #print output

    # Kill app
    #core_channel.end()
    output = get_stdout(cmd_kill_core_app)
    print output

