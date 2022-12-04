sudo mkdir /media/usb
sudo printf "\nUUID=$(ls -l /dev/disk/by-uuid | egrep sd* | egrep -o '[0-9a-f]{8}-([0-9a-f]{4}-){3}[0-9a-f]{12}') /media/usb ext4 defaults,nofail 0 0" | sudo tee -a /etc/fstab
sudo reboot now