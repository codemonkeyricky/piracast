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

import re
import time

from util import get_stdout

cmd_killall_wpa_spplicant   = 'killall wpa_supplicant'
cmd_killall_hostapd         = 'killall hostapd'
cmd_iwlist_wlan0_scan       = 'iwlist wlan0 scan'

def peer_mac_get():
    output = get_stdout('iwpriv wlan0 p2p_get peer_ifa')
    match = re.search(r'MAC (.*)$', output)
    return match.group(1)

def wpa_supplicant_start():
    print 'wpa_supplicant_start:'
    get_stdout(["./wpa_supplicant", "-i", "wlan0", "-c", "./wpa_0_8.conf", "-B"])
    time.sleep(1)

def wps_auth():
    print 'wps_auth:'
    output = get_stdout(["./hostapd_cli", "wps_pbc", "any"])
    print output
    time.sleep(1)

def wps_status_get():
    print 'wps_status_get:'
    output = get_stdout(["./wpa_cli", "status"])
    print output

def p2p_wpsinfo():
    print 'p2p_wpsinfo:'
    get_stdout('iwpriv wlan0 p2p_set got_wpsinfo=3')

def p2p_status_get():
    #print 'p2p_status_get:'
    output = get_stdout('iwpriv wlan0 p2p_get status')
    match = re.search(r'Status=(\d*)', output)
    return int(match.group(1))

def p2p_set_nego(mac):
    print 'p2p_set_nego:'
    print 'mac: %s' % mac
    get_stdout('iwpriv wlan0 p2p_set nego=%s' % mac)

    # Enter negotiation loop
    while 1:

        # Wait for result
        time.sleep(0.5)

        # Poll status
        peer_status = p2p_status_get()
        print 'peer_status: %d' % peer_status

        # For Windows 8.1 support, we consider 19 as negotiation completed
        if peer_status in [10, 19]:
            print 'Negotiation suceeded!'
            break

    # Get role
    role = p2p_role_get()
    print 'Role: %s' % role

    # TODO: doesn't seem to return anything
    #p2p_opch_get()

    # Get peer interface address
    peer_mac_get()

    p2p_go_mode_set()

# -----------------------
# p2p_enable
#   Enable wifi direct
# -----------------------
def p2p_enable():

    # Enable p2p
    get_stdout('iwpriv wlan0 p2p_set enable=1')

    # Set intent
    get_stdout('iwpriv wlan0 p2p_set intent=15')

    # Set operation channel
    get_stdout('iwpriv wlan0 p2p_set op_ch=%d' % 11)

    # Sleep for 50ms
    time.sleep(0.05)

    # Set ssid
    get_stdout('iwpriv wlan0 p2p_set ssid=DIRECT-RT')

    # Set DN
    get_stdout('iwpriv wlan0 p2p_set setDN=Piracast')

    #print 'p2p_get role...'
    #get_stdout('iwpriv wlan0 p2p_get role')

    #print 'scan...'
    #get_stdout('iwlist wlan0 scan')

# -----------------------
# p2p_peer_devaddr_get
#   Gets peer device address
# -----------------------
def p2p_peer_devaddr_get():
    print 'p2p_peer_devaddr_get:'
    output = get_stdout(["iwpriv", "wlan0", "p2p_get", "peer_deva"])
    match = re.search(r'\n(.*)$', output)
    mac = ':'.join(re.findall('..', match.group(1)))
    #mac = match.group(1)[0] + match.group(1)[1] + ':' \
    #    + match.group(1)[2] + match.group(1)[3] + ':' \
    #    + match.group(1)[4] + match.group(1)[5] + ':' \
    #    + match.group(1)[6] + match.group(1)[7] + ':' \
    #    + match.group(1)[8] + match.group(1)[9] + ':' \
    #    + match.group(1)[10] + match.group(1)[11]

    return mac

# -----------------------
# p2p_req_cm_get
#   Gets supported authentication type
# -----------------------
def p2p_req_cm_get():
    print 'p2p_req_cm_get:'
    print get_stdout('iwpriv wlan0 p2p_get req_cm')

def p2p_role_get():
    print 'p2p_role_get:'
    output = get_stdout('iwpriv wlan0 p2p_get role')
    match = re.search(r'Role=(\d*)', output)
    role = int(match.group(1))
    return role

def p2p_opch_get():
    print 'p2p_opch_get:'
    print '---------------------------'
    output = get_stdout('iwpriv wlan0 p2p_get op_ch')
    print output
    print '---------------------------'
    #match = re.search(r'Role=(\d*)', output)
    #role = int(match.group(1))
    #return role

def p2p_go_mode_set():

    # Start hostAPd and wait for it to daemonize; ignore stdout
    get_stdout(["./hostapd", "-B", "p2p_hostapd.conf"])

    # Wait for initialization
    time.sleep(1)

    do_wps()

    # Wait for host apd interval
    time.sleep(1)

    while 1:
        status = read_all_sta()

        if status:
            print 'Wireless display negotiation completed!'
            break

        time.sleep(1)

def do_wps():

    while 1:
        print 'do_wps:'
        output = get_stdout(["./hostapd_cli", "wps_pbc", "any"])

        print output

        if 'OK' in output:
            print 'wps passed!'
            return

        time.sleep(1)

def read_all_sta():
    print 'read_all_sta:'
    output = get_stdout(["./hostapd_cli", "all_sta"])
    return ('dot11RSNAStatsSTAAddress' in output)

def p2p_disable():
    get_stdout('iwpriv wlan0 p2p_set enable=0')

def p2p_peer_scan():
    count = 0

    while 1:
        output = get_stdout(cmd_iwlist_wlan0_scan)

        print output

        if 'No scan results' not in output:
            return True

        if count > 3:
            return False

        count += 1

def wfd_connection_wait():
    get_stdout(cmd_killall_wpa_spplicant)
    get_stdout(cmd_killall_hostapd)

    # Disable p2p
    p2p_disable()

    time.sleep(0.5)

    # Enable p2p
    p2p_enable()

    #p2p_peer_scan()

    print 'Waiting for incoming connection...'

    while 1:

        peer_status = p2p_status_get()

        print 'peer_status: %d' % peer_status

        if peer_status == 0:
            print 'p2p disabled! Re-enable p2p...'
            p2p_enable()

        #if peer_status == 11:
        #    print 'p2p request received! Scan for peer ...'
        #    p2p_peer_scan()

        # status 8 is the original Discovery Request
        # status 22 needs to be handled this way, or Nexus 4 4.4 won't always work
        # status 19 was added to try to implement windows 8.1 support
        if peer_status in [8, 19, 22]:
            # Discovery request or gonego fail
            print 'Discovery request received!'
            peer_found = p2p_peer_scan()

            if peer_found:
                break

            p2p_disable()

        time.sleep(1)

    print 'Getting peer device address...'

    # Get peer device address
    mac = p2p_peer_devaddr_get()
    print 'peer_devaddr: %s' % mac

    # Notify received wps info
    p2p_wpsinfo()

    print 'Getting peer authentication type...'

    # Get request configuration
    p2p_req_cm_get()

    print 'Confirming peer authentication...'

    #print 'Getting status...'

    # Get status
    #peer_status = p2p_status_get()
    #print 'peer_status: ', peer_status

    # Set negotiation
    p2p_set_nego(mac)
