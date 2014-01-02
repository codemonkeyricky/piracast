import os
import re
import subprocess
from subprocess import call

import wfd
import core_channel
import sink

# Getting the leased IP address.
def leased_ip_get() : 

    contents = open("/var/lib/dhcp/dhcpd.leases").read()

    ip_list = re.findall(r'lease (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', contents)

    # Return the most recent leased IP. 
    return ip_list[-1]

def lease_file_timestamp_get() : 

    cmd_ls_dhcp_lease = 'ls -l /var/lib/dhcp/dhcpd.leases'

    rsp = subprocess.Popen(cmd_ls_dhcp_lease.split(), shell=False, stdout=subprocess.PIPE)
    ts = rsp.stdout.read(); 

    return ts; 

cmd_dhcp_start = 'service isc-dhcp-server start'
cmd_dhcp_stop = 'service isc-dhcp-server stop'
# cmd_kill_app = 'python core_terminate.py'
cmd_kill_app = 'killall app'
cmd_wlan0_up = 'ifup wlan0'

print 'Bring up wlan0 just in case...' 
subprocess.Popen(cmd_wlan0_up.split(), shell=False, stdout=subprocess.PIPE)

print 'Increase rmem_default...'
cmd = 'sudo sysctl -w net.core.rmem_default=1000000'
subprocess.Popen(cmd.split(), shell=False, stdout=subprocess.PIPE)

print 'Kill running application...'
# core_channel.end(); 
console_output = subprocess.Popen(cmd_kill_app.split(), shell=False, stdout=subprocess.PIPE)
output = console_output.stdout.read(); 
print output

while True : 
    
    # Launch application.
    cmd = 'sudo nice -n -20 ./app'
    subprocess.Popen(cmd.split(), shell=False, stdout=open(os.devnull, 'w'))

    # Start DHCP. 
    console_output = subprocess.Popen(cmd_dhcp_start.split(), shell=False, stdout=subprocess.PIPE)
    output = console_output.stdout.read(); 
    print output

    # Get previous timestamp. 
    prev_ts = lease_file_timestamp_get(); 
    
    # Wait for connection.
    wfd.wfd_connection_wait() 

    # Wait until lease file is updated. 
    while True : 
        
        curr_ts = lease_file_timestamp_get(); 

        if curr_ts != prev_ts : 

            print 'Source has requested IP!'

            # wait for network to be properly configured. 
            call(["sleep", "2"])

            break; 

        print 'lease table has not been updated, wait for a second...' 

        call(["sleep", "1"])
    
    # Get source IP. 
    ip = leased_ip_get(); 
    
    print 'leasd IP: ', ip
    
    # Connect to source.
    sink.source_connect(ip); 
    
    # Stop DHCP. 
    console_output = subprocess.call(cmd_dhcp_stop.split(), shell=False, stdout=subprocess.PIPE)
    # output = console_output.stdout.read(); 
    #print output

    # Kill app.
    # core_channel.end(); 
    console_output = subprocess.Popen(cmd_kill_app.split(), shell=False, stdout=subprocess.PIPE);
    output = console_output.stdout.read(); 
    print output
