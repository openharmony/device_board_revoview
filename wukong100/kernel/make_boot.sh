#!/bin/bash

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

set -e
set -x
#$1 - kernel build script work dir
#$2 - kernel build script stage dir
#$3 - GN target output dir

ramdisk_name=ramdisk.img
boot_name=boot.img
vendor_boot_name=vendor_boot.img

if [ $# -gt 0 ] && [ "${1}" == "updater" ] ; then
    vendor_boot_name=vendor_boot_updater.img
    ramdisk_name=updater.img
    boot_name=boot-updater.img
else
    #build and copy dtbo.img to out directory
    dtbo_name=mkboot/dist/dtbo.img
	dtb_name=mkboot/dist/dtb.img
    rm -f ${dtbo_name}
	rm -f ${dtb_name}
    ./mkboot/bin/mkdtimg create ./${dtbo_name} ./mkboot/dist/uis7885-2h10-overlay.dtbo --id=0x220000
    cp -f ${dtbo_name} ../../../packages/phone/images/.
    
    ./mkboot/bin/mkdtimg create ./${dtb_name} ./mkboot/dist/*.dtb
    cp -f ${dtb_name} ../../../packages/phone/images/.
fi

ramdisk_path=./mkboot/dist/${ramdisk_name}
rm -f ${boot_name}

./mkboot/bin/mkbootimg --kernel ./mkboot/dist/Image \
--ramdisk ${ramdisk_path} --base 0x00000000 --pagesize 4096 \
--cmdline "console=ttyS1,115200n8 buildvariant=engdebug" \
--os_version 13 --os_patch_level 2023-03-05 --kernel_offset 0x00008000 \
--ramdisk_offset 0x05400000 --header_version 4 -o ./${boot_name}

./mkboot/bin/mkbootimg --dtb ./mkboot/dist/dtb.img --base 0x00000000 --pagesize 4096 \
--vendor_cmdline "console=ttyS1,115200n8 buildvariant=engdebug" \
--kernel_offset 0x00008000 --ramdisk_offset 0x05400000 --header_version 4 \
--pagesize=4096 --vendor_ramdisk ${ramdisk_path} --vendor_boot ./${vendor_boot_name}


cp -f ${boot_name} ../../../packages/phone/images/.
cp -f ${vendor_boot_name} ../../../packages/phone/images/.
