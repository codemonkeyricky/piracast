#!/usr/bin/env python3

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

# Getting the leased IP address
def leased_ip_get():
    contents = open("/var/lib/dhcp/dhcpd.leases").read()
    ip_list = re.findall(r'lease (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', contents)

    # Return the most recent leased IP
    return ip_list[-1]

def lease_file_timestamp_get():
    return get_stdout('ls -l /var/lib/dhcp/dhcpd.leases')

cmd_dhcp_start = 'service isc-dhcp-server start'
cmd_dhcp_stop = 'service isc-dhcp-server stop'
#cmd_kill_core_app = 'python core_terminate.py'
cmd_kill_core_app = 'killall core'
cmd_wlan0_up = 'ifup wlan0'

print 'Bring up wlan0 just in case...'
get_stdout(cmd_wlan0_up)

print 'Increase rmem_default...'
get_stdout('sudo sysctl -w net.core.rmem_default=1000000')

print 'Kill running application...'
#core_channel.end()
print get_stdout(cmd_kill_core_app)

while 1:

    # Launch application.
    get_stdout('sudo nice -n -20 ./core')

    # Start DHCP. 
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

    # Connect to source
    sink.source_connect(ip)

    # Stop DHCP
    output = get_stdout(cmd_dhcp_stop)
    #print output

    # Kill app
    #core_channel.end()
    output = get_stdout(cmd_kill_core_app)
    print output

