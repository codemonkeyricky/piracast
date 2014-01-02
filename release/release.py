# Piracast project release script. 
#   This script generates the release package for release. 

import os
import shutil
import subprocess

# Change to target directory.
os.chdir("../target") 
 
# Compile application. 
cmd_make_app = 'make clobber app'
os.system(cmd_make_app)
 
# Change to application directory.
os.chdir("../application") 
 
# Freeze main. 
print 'Freezing python application...'
cmd_freeze_main = 'cxfreeze main.py'
os.system(cmd_freeze_main)
 
# Go back to release directory. 
os.chdir("../release") 

print 'Creating directories...'
cmd_mkdir_piracast = 'mkdir piracast'
os.system(cmd_mkdir_piracast)
 
os.chdir("./piracast") 
cmd_mkdir_application = 'mkdir application'
os.system(cmd_mkdir_application)
 
cmd_mkdir_installer = 'mkdir installer'
os.system(cmd_mkdir_installer)
 
# Go back to release directory.
os.chdir("../") 
 
# Copy application files. 
paths = ['../application/hostapd', 
         '../application/hostapd_cli', 
         '../application/p2p_hostapd.conf', 
         '../application/wpa_0_8.conf', 
         '../application/wpa_cli',
         '../application/wpa_supplicant', 
         '../application/app', 
         '../application/dist/main']
 
for path in paths : 
   shutil.copy(path, 'piracast/application') 
   
# # Enable executables. 
# executables = ['piracast/application/hostapd', 
#          'piracast/application/hostapd_cli', 
#          'piracast/application/wpa_cli',
#          'piracast/application/wpa_supplicant', 
#          'piracast/application/app']
#  
# for executable in executables : 
#     enable_add_permission_cmd = 'chmod 777 ' + executable
#     os.system(enable_add_permission_cmd)
    
# Copy installer files.
installs = ['../environment/8188eu.ko', 
            '../environment/isc-dhcp-server', 
            '../environment/dhcpd.conf', 
            '../environment/interfaces', 
            '../environment/setup.py']
 
for install in installs : 
   shutil.copy(install, 'piracast/installer') 
   
# Copy instruction files.
instructions = [
            '../environment/README'
               ]
 
for instruction in instructions : 
   shutil.copy(instruction, 'piracast') 
    
print 'Create tar package...'
cmd_tar_piracast = 'tar -zcvf piracast.tar.gz piracast'
os.system(cmd_tar_piracast)
 
print 'Release package ready!' 
