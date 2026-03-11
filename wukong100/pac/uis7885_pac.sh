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
set -v

curr=$(pwd -P)
#basepath=$(cd `dirname $0`; pwd)
basepath=$2
script_path=$2/Script
secboot=0
# args from build.sh or from bash execue
if [[ $# -ge 1 ]]; then
package_path=$1
else
package_path=`cd ../../../../../out/wukong100/packages/phone/images; pwd`
fi

# secboot
if [[ $# -ge 3 ]]; then
  if [[ $3 -eq "secboot" ]]; then
    secboot=1
  fi
fi

# some prebuild img here
if [ $secboot -eq 1 ]; then
    #work_path=$basepath/ImageFiles_secboot/
    work_path=$basepath/
    # we output the pac to this dest path
    target_path=$package_path/wukong100_sec_userdebug.tar.gz
    pac_path=$package_path/wukong100_sec_userdebug.pac
else
    work_path=$basepath/
    # we output the pac to this dest path
    target_path=$package_path/wukong100_nosec_userdebug.tar.gz
    pac_path=$package_path/wukong100_nosec_userdebug.pac
fi

echo curr path: $curr
echo package path: $package
echo target path: $work_path

# del it first
if [[ -f "$target_path" ]]; then
    rm -f $target_path
fi

# cp image to work path
cp $package_path/*.img $work_path -rfv
#if [ $secboot -eq 1 ]; then
#    cp $package_path/*.bin $work_path/secboot_img -rfv
#else
#    cp $package_path/*.bin $work_path -rfv
#fi

# hvbtool sign image
if [ $secboot -eq 1 ]; then
    echo 'sign image start...'
    bash ${curr}/../../device/board/revoview/wukong100/pac/create_sign_img_all.sh
fi

# go to work path and make pac
cd $work_path
if [ $secboot -eq 1 ]; then
    perl $script_path/mkpac.pl "$pac_path" "flash_secboot.cfg" "FlashParam"
else
    perl $script_path/mkpac.pl "$pac_path" "flash.cfg" "FlashParam"
fi

if [ $secboot -eq 1 ]; then
    tar -czf $target_path -C $package_path wukong100_sec_userdebug.pac
else
    tar -czf $target_path -C $package_path wukong100_nosec_userdebug.pac
fi

# check if pac is done
if [[ -f "$target_path" ]]; then
#    rm $pac_path
    echo -e "\033[32m  wukong100 build pac successful. \033[0m"
fi
