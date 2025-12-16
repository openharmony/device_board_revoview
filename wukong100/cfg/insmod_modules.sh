#/bin/sh
insmod /vendor/modules/sprd_wlan_combo.ko
insmod /vendor/modules/sprdbt_tty.ko
sleep 10
chmod 775 /sys/class/rfkill/rfkill0/state
chown blue_host:blue_host /sys/class/rfkill/rfkill0/state
chown blue_host:blue_host /dev/ttyBT0
chown blue_host:blue_host /dev/ttyBT1
chown blue_host:blue_host /proc/bluetooth/sleep/lpm
chown blue_host:blue_host /proc/bluetooth/sleep/btwrite
chown bluetooth:bluetooth /dev/uhid
