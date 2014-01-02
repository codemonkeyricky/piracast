import re
import subprocess
from subprocess import call

# Command
cmd_ls_dhcp_lease = 'ls -l /var/lib/dhcp/dhcpd.leases'

rsp = subprocess.Popen(cmd_ls_dhcp_lease.split(), shell=False, stdout=subprocess.PIPE)
prev_ts = rsp.stdout.read(); 

while True : 

    rsp = subprocess.Popen(cmd_ls_dhcp_lease.split(), shell=False, stdout=subprocess.PIPE)
    curr_ts = rsp.stdout.read(); 

    if curr_ts != prev_ts : 

        print 'file udpated!'

        break; 

    

