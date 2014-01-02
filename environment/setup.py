import os

print 'Installing wireless driver...'
cmd_copy_driver = 'cp 8188eu.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless'
os.system(cmd_copy_driver)
os.system('depmod -a') 
os.system('modprobe 8188eu') 

print 'Installing DHCP server...'
cmd_apt_get_update = 'sudo apt-get update'
cmd_install_dhcp = 'sudo apt-get install isc-dhcp-server'
os.system(cmd_apt_get_update)
os.system(cmd_install_dhcp)

print 'Overwriting isc-dhcp-server ...' 
cmd_copy_dhcp_server = 'cp isc-dhcp-server /etc/default/'
os.system(cmd_copy_dhcp_server)

print 'Overwriting dhcpd.conf ...' 
cmd_copy_dhcp_server = 'cp dhcpd.conf /etc/dhcp/'
os.system(cmd_copy_dhcp_server)

print 'Overwriting network interface file (interfaces) ...' 
cmd_copy_interfaces = 'cp interfaces /etc/network/'
os.system(cmd_copy_interfaces)

print 'Enable execute permission executables...' 

executables = ['../application/hostapd', 
         '../application/hostapd_cli', 
         '../application/wpa_cli',
         '../application/wpa_supplicant', 
         '../application/app']
  
for executable in executables : 
    enable_add_permission_cmd = 'chmod 777 ' + executable
    os.system(enable_add_permission_cmd)

print ''
print ''
print ''

print 'Installation is completed. Please restart your Pi.'
print ''
print 'To launch the application after restart:'
print '    1. cd piracast/application'
print '    1. sudo ./main'
print ''
