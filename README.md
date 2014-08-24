Welcome to the Piracast project. 
=========
Copyright 2014 


### Limitation: 
    1. Only works with TP-Link dongle.
    2. No HDCP support (cannot remote Netflix or Google Music). 

### Install Driver:
    1. uname -r //gives ur rp version number 
    2. Based on the rp version number choose ur 8188eu tar file from http://www.raspberrypi.org/forums/viewtopic.php?p=462982#p462982
    3. wget https://dl.dropboxusercontent.com/u/80256631/8188eu-201xyyzz.tar.gz
    4. tar -zxvf 8188eu-201*.tar.gz
    5. sudo cp rtl8188eufw.bin /lib/firmware/rtlwifi //not needed anymore if your operating system is above 3.10.33+
    6. sudo install -p -m 644 8188eu.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless
    7. sudo insmod /lib/modules/3.10.33+/kernel/drivers/net/wireless/8188eu.ko
    8. sudo depmod -a
    9. sudo reboot
    

### Install DHCP server
    1. sudo apt-get install isc-dhcp-server
    2. sudo cp env/isc-dhcp-server /etc/default
    3. sudo cp env/dhcpd.conf /etc/dhcp/
    4. sudo cp env/interfaces /etc/network/interfaces

### Compile the project: 
    1. cd target
    2. make core

### Reboot after installation is completed:
    1. sudo reboot

### To run Piracast:
    1. cd scripts
    2. sudo python piracast.py
