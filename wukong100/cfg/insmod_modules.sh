# Copyright (C) 2023 HiHope Open Source Organization.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
