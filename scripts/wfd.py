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
#

import re
import subprocess
from subprocess import call

def peer_mac_get() :
    rsp = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "peer_ifa"], shell=False, stdout=subprocess.PIPE)
    output = rsp.stdout.read(); 
    match = re.search(r'MAC (.*)$', output)
    return match.group(1)

def wpa_supplicant_start() : 
    print 'wpa_supplicant_start:'
    call(["./wpa_supplicant", "-i", "wlan0", "-c", "./wpa_0_8.conf", "-B"])
    call(["sleep", "1"])

def wps_auth() : 
    print 'wps_auth:'
    rsp = subprocess.Popen(["./hostapd_cli", "wps_pbc", "any"], shell=False, stdout=subprocess.PIPE)
    output = rsp.stdout.read(); 
    print output
    call(["sleep", "1"])

def wps_status_get() : 
    print 'wps_satus_get:'
    rsp = subprocess.Popen(["./wpa_cli", "status"], shell=False, stdout=subprocess.PIPE)
    output = rsp.stdout.read(); 
    print output

def p2p_wpsinfo() : 
    print 'p2p_wpsinfo:'
    call(["iwpriv", "wlan0", "p2p_set", "got_wpsinfo=3"])

def p2p_status_get() : 
#     print 'p2p_status_get:'
    rsp = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "status"], shell=False, stdout=subprocess.PIPE)
    output = rsp.stdout.read(); 
    match = re.search(r'Status=(\d*)', output)
    peer_status = int(match.group(1))
    return peer_status

def p2p_set_nego(mac) : 
    print 'p2p_set_nego:'
    print 'mac: ', mac
    call(["iwpriv", "wlan0", "p2p_set", "nego=" + mac])

    # Enter negotiation loop.
    while True : 

        # Wait for result. 
        call(["sleep", ".5"])

        # Poll status.
        peer_status = p2p_status_get()
        print 'peer_status: ', peer_status 

        if peer_status == 10 : 
            print 'Negotiation suceeded!' 
            break; 
        
    # Get role
    role = p2p_role_get()
    print 'Role: ', role

    # TODO: doesn't seem to return anything
    p2p_opch_get(); 

    # Get peer interface address.
    peer_mac_get(); 

    p2p_go_mode_set(); 

# ----------------------- 
# p2p_enable
#   Enable wifi direct. 
# ----------------------- 
def p2p_enable() : 

    # Enable p2p
    call(["iwpriv", "wlan0", "p2p_set", "enable=1"])

    # Set intent
    call(["iwpriv", "wlan0", "p2p_set", "intent=15"])

    # Set operation channel
    call(["iwpriv", "wlan0", "p2p_set", "op_ch=9"])

    # Sleep for 50ms
    call(["sleep", "0.05"])

    # Set ssid
    call(["iwpriv", "wlan0", "p2p_set", "ssid=DIRECT-RT"])

    # Set DN
    call(["iwpriv", "wlan0", "p2p_set", "setDN=Piracast"])

    # print 'p2p_get role...'
    # call(["iwpriv", "wlan0", "p2p_get", "role"])

#     print 'scan...'
#     call(["iwlist", "wlan0", "scan"])

# ----------------------- 
# p2p_peer_devaddr_get
#   Gets peer device address
# ----------------------- 
def p2p_peer_devaddr_get() : 
    print 'p2p_peer_devaddr_get:'
    console_output = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "peer_deva"], shell=False, stdout=subprocess.PIPE)
    match = re.search(r'\n(.*)$', console_output.stdout.read())
    mac = match.group(1)[0] + match.group(1)[1] + ':' \
        + match.group(1)[2] + match.group(1)[3] + ':' \
        + match.group(1)[4] + match.group(1)[5] + ':' \
        + match.group(1)[6] + match.group(1)[7] + ':' \
        + match.group(1)[8] + match.group(1)[9] + ':' \
        + match.group(1)[10] + match.group(1)[11]

    return mac; 

# ----------------------- 
# p2p_req_cm_get
#   Gets supported authentication type.
# ----------------------- 
def p2p_req_cm_get() : 
    print 'p2p_req_cm_get:'
    console_output = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "req_cm"], shell=False, stdout=subprocess.PIPE)
    print console_output.stdout.read(); 

# ----------------------- 
# p2p_req_cm_get
#   Gets supported authentication type.
# ----------------------- 
def p2p_req_cm_get() : 
    print 'p2p_req_cm_get:'
    console_output = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "req_cm"], shell=False, stdout=subprocess.PIPE)
    print console_output.stdout.read(); 

# ----------------------- 
# p2p_req_cm_get
#   Gets supported authentication type.
# ----------------------- 
def p2p_role_get() : 
    print 'p2p_role_get:'
    console_output = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "role"], shell=False, stdout=subprocess.PIPE)
    match = re.search(r'Role=(\d*)', console_output.stdout.read())
    role = int(match.group(1))
    return role

def p2p_opch_get() : 
    print 'p2p_opch_get:'
    print '---------------------------'
    console_output = subprocess.Popen(["iwpriv", "wlan0", "p2p_get", "op_ch"], shell=False, stdout=subprocess.PIPE)
    print console_output.stdout.read()
    print '---------------------------'
    # match = re.search(r'Role=(\d*)', console_output.stdout.read())
    # role = int(match.group(1))
    # return role

def wait_forever() : 

    while True : 

        call(["sleep", "1"])

def p2p_go_mode_set() : 

    # Start host APD
    call(["./hostapd", "-B", "p2p_hostapd.conf"])

    # Wait for initialization.
    call(["sleep", "1"])

    do_wps(); 

    # Wait for host apd interval
    call(["sleep", "1"])

    while True : 

        status = read_all_sta()

        if status == True : 
            print 'Wireless display negotiation completed!'
            break; 

        call(["sleep", "1"])

def do_wps() : 

    while (1) : 

        print 'do_wps'
        console_output = subprocess.Popen(["./hostapd_cli", "wps_pbc", "any"], shell=False, stdout=subprocess.PIPE)

        status_ok = "OK"
        output = console_output.stdout.read()

        print output

        if 'OK' in output : 
            print 'wps passed!' 
            break; 

        call(["sleep", "1"])

def read_all_sta() : 

    print 'read_all_sta:'
    console_output = subprocess.Popen(["./hostapd_cli", "all_sta"], shell=False, stdout=subprocess.PIPE)
    output = console_output.stdout.read()

    print output

    if 'dot11RSNAStatsSTAAddress' in output : 
        return True; 

    return False; 

def p2p_disable() : 

    call(["iwpriv", "wlan0", "p2p_set", "enable=0"])
    
def p2p_peer_scan() :
    
    count = 0; 
    
    while True : 
        
        console_output = subprocess.Popen(cmd_iwlist_wlan0_scan.split(), shell=False, stdout=subprocess.PIPE)
        output = console_output.stdout.read(); 
        
        print output
        
        if 'No scan results' not in output :
            
            return True; 
        
        if count > 3 :
            
            return False; 
            
        count += 1; 
        

# ----------------------- 
#   MAIN
# ----------------------- 

cmd_killall_wpa_spplicant   = 'killall wpa_supplicant'
cmd_killall_hostapd         = 'killall hostapd'
cmd_iwlist_wlan0_scan       = 'iwlist wlan0 scan'

def wfd_connection_wait() : 
    
    call(cmd_killall_wpa_spplicant.split())

    # Kill app 
    call(cmd_killall_hostapd.split())
    
    # Disable p2p
    p2p_disable(); 
    
    call(["sleep", "0.5"])
    
    # Enable p2p
    p2p_enable() ; 
    
#     p2p_peer_scan() ; 

    print 'Waiting for incoming connection...'
    
    while (1) : 
    
        peer_status = p2p_status_get()
        
        print 'peer_status: ', peer_status
    
        if peer_status == 0 : 
            
            print 'p2p disabled! Re-enable p2p...'

            p2p_enable() ; 
        
#         if peer_status == 11 : 
#              
#             print 'p2p request received! Scan for peer ...'
#  
#             p2p_peer_scan() ; 
    
        if peer_status == 8 : 
    
            # Discovery request or gonego fail. 
            print 'Discovery request received!'
            
            peer_found = p2p_peer_scan() ;
            
            if peer_found == False : 

                p2p_disable(); 
                
            else : 
                
                break
    
        call(["sleep", "1"])
        
    print 'Getting peer device address...'
    
    # Get peer device address.
    mac = p2p_peer_devaddr_get(); 
    print 'peer_devaddr: ', mac
    
    # Notify received wps info. 
    p2p_wpsinfo(); 
    
    print 'Getting peer authentication type...'
    
    # Get request configuration. 
    p2p_req_cm_get(); 
    
    print 'Confirming peer authentication...'
    
#     print 'Getting status...'
    
    # Get status. 
#     peer_status = p2p_status_get()
#     print 'peer_status: ', peer_status 
    
    # Set negotiation. 
    p2p_set_nego(mac)
    
