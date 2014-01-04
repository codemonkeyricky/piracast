Welcome to the Piracast project. 
=========
Copyright 2013

### Limitation: 
    1. Only works with TP-Link dongle.
    2. No HDCP support (cannot remote Netflix or Google Music). 

### Install Driver:
	1. sudo cp env/8188eu.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless
   	2. sudo depmod -a
   	3. sudo modprobe 8188eu

### Install DHCP server
	1. sudo apt-get install isc-dhcp-server
	2. cp env/isc-dhcp-server /etc/default
	3. cp dhcpd.conf /etc/dhcp/

### Compile the project: 
    1. cd target
    2. make core

### To run Piracast:
    1. cd scripts
    2. sudo python piracast.py
