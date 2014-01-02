import re

# test_payload = 'selected interface \'wlan0\'\n' \
#                'OK'

contents = open("/var/lib/dhcp/dhcpd.leases").read()

match = re.findall(r'lease (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', contents)

if match : 
    print 'found!'
    print match[-1]

else : 
    print 'not found!'

print contents

