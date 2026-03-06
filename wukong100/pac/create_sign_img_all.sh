#! /bin/bash
# Copyright (C) 2026 Beijing Zhongke Honglve Technology Co., Ltd. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0+
#
# This software is proprietary to Zhongke Honglve Technology Co., Ltd. and may only be used
# in accordance with the terms of the GNU General Public License version 3 or any later version.
# Unauthorized copying, modification, or distribution of this software is strictly prohibited.

set -e
set -v

curr=$(pwd -P)
basepath=$(cd `dirname $0`; pwd)
key_path="${basepath}/secureboot_key/config"
package_path="${basepath}/../../../../../out/wukong100/packages/phone/images"
img_path="${basepath}/../../../../../device/soc/unisoc/p7885//pac/ImageFiles/secboot_img"
cmd_sign="python3 ${basepath}/../../../../../base/startup/hvb/tools/hvbtool.py"
rollback_index=0

echo curr path: $curr
echo basepath: $basepath
echo package path: $package_path
echo cmd path: $cmd_sign

#hash 签名
if [ -f ${package_path}/dtbo.img ]; then
    rm -rf ${img_path}/signed_dtbo.img
    $cmd_sign make_hash_footer --image ${package_path}/dtbo.img --partition dtbo_a --partition_size 8388608 --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public_boot.pem --privkey ${key_path}/rsa2048_private_boot.pem --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 1 --output ${img_path}/signed_dtbo.img
else
    echo "${package_path}/dtbo.img does not exist.\n"
    exit 1
fi


#hashtree签名

if [ -f ${package_path}/boot.img ]; then
    rm -rf ${img_path}/signed_boot.img
    $cmd_sign make_hashtree_footer --image ${package_path}/boot.img --partition boot_a --partition_size 67108864 --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public_boot.pem --privkey ${key_path}/rsa2048_private_boot.pem --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 4 --output ${img_path}/signed_boot.img
else
    echo "${package_path}/boot.img does not exist.\n"
    exit 1
fi

if [ -f ${package_path}/vendor_boot.img ]; then
    rm -rf ${img_path}/signed_vendor_boot.img
    $cmd_sign make_hashtree_footer --image ${package_path}/vendor_boot.img --partition vendor_boot_a --partition_size 104857600 --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public_vendor_boot.pem --privkey ${key_path}/rsa2048_private_vendor_boot.pem --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 5 --output ${img_path}/signed_vendor_boot.img
else
    echo "${package_path}/vendor_boot.img does not exist.\n"
    exit 1
fi

if [ -f ${package_path}/system.img ]; then
    rm -rf ${img_path}/signed_system.img
    $cmd_sign make_hashtree_footer --image ${package_path}/system.img --partition system --partition_size 3145728000 --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public_system.pem --privkey ${key_path}/rsa2048_private_system.pem --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 6 --output ${img_path}/signed_system.img
else
    echo "${package_path}/system.img does not exist.\n"
    exit 1
fi

if [ -f ${package_path}/vendor.img ]; then
    rm -rf ${img_path}/signed_vendor.img
    $cmd_sign make_hashtree_footer --image ${package_path}/vendor.img --partition vendor --partition_size 838860800 --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public_vendor.pem --privkey ${key_path}/rsa2048_private_vendor.pem --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 7 --output ${img_path}/signed_vendor.img
else
    echo "${package_path}/vendor.img does not exist.\n"
    exit 1
fi

#rvt生成
rm -rf ${img_path}/rvt.img
$cmd_sign make_rvt_image --salt 9f3a79b7f2bad5adb086bcb8cf37f991733f2696 --pubkey ${key_path}/rsa2048_public.pem --privkey ${key_path}/rsa2048_private.pem --partition rvt --partition_size 1048576 --algorithm SHA256_RSA2048 --rollback_index $rollback_index --rollback_location 0 --chain_partition boot_a:${key_path}/rsa2048_public_boot.pem --chain_partition vendor_boot_a:${key_path}/rsa2048_public_vendor_boot.pem --chain_partition dtbo_a:${key_path}/rsa2048_public_boot.pem  --chain_partition system:${key_path}/rsa2048_public_system.pem --chain_partition vendor:${key_path}/rsa2048_public_vendor.pem --output ${img_path}/rvt.img
