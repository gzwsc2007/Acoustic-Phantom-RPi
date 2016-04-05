sudo cp /etc/dhcpcd.conf.master /etc/dhcpcd.conf
sudo cp /etc/network/interfaces.master /etc/network/interfaces
sudo update-rc.d hostapd enable
sudo update-rc.d dnsmasq enable
