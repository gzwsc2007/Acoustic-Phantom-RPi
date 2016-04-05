sudo service hostapd stop
sudo service dnsmasq stop
sudo cp /etc/dhcpcd.conf.slave /etc/dhcpcd.conf
sudo cp /etc/network/interfaces.slave /etc/network/interfaces
sudo service dhcpcd restart
sudo systemctl daemon-reload
sudo service networking restart
sudo update-rc.d hostapd disable
sudo update-rc.d dnsmasq disable
