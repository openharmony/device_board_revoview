#! /bin/bash
# Copyright (C) 2026 Beijing Zhongke Honglve Technology Co., Ltd.
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

BASEPATH=$(cd `dirname $0`; pwd)
KEY_PATH="${BASEPATH}/secureboot_key/config"
PACKAGE_PATH="${BASEPATH}/../../../../../device/soc/unisoc/p7885/pac/ImageFiles"
IMG_PATH="${BASEPATH}/../../../../../device/soc/unisoc/p7885/pac/ImageFiles/secboot_img"
KERNEL_PATH="${BASEPATH}/../../../../../kernel_unisoc_p7885"
# 签名工具与固定参数
CMD_SIGN="/usr/bin/python3 ${BASEPATH}/../../../../../base/startup/hvb/tools/hvbtool.py"
ROLLBACK_INDEX=0
ALGORITHM="SHA256_RSA2048"

echo basepath: $BASEPATH
echo package path: $PACKAGE_PATH
echo img path: $IMG_PATH
echo cmd path: $CMD_SIGN

# 功能：通用镜像签名逻辑（支持hash/hashtree两种签名类型）
# 参数：
#   $1: 签名类型（hash/hashtree）
#   $2: 源镜像文件名（如 dtbo.img/boot.img）
#   $3: 分区名（如 dtbo_a/boot_a）
#   $4: 分区大小（如 8388608/67108864）
#   $5: 公钥文件名（如 rsa2048_public_boot.pem）
#   $6: 私钥文件名（如 rsa2048_private_boot.pem）
#   $7: rollback_index（如 1/4）
#   $8: rollback_location（如 1/4）
#   $9: 输出镜像名（如 signed_dtbo.img/signed_boot.img）
sign_image() {
    local sign_type="$1"
    local src_img="$2"
    local partition="$3"
    local part_size="$4"
    local pubkey="$5"
    local privkey="$6"
    local rollback_ind="$7"
    local rollback_loc="$8"
    local output_img="$9"

    # 1. 拼接完整路径
    local src_full="${PACKAGE_PATH}/${src_img}"
    local pubkey_full="${KEY_PATH}/${pubkey}"
    local privkey_full="${KEY_PATH}/${privkey}"
    local output_full="${IMG_PATH}/${output_img}"

    # 2. 基础检查（源镜像+密钥）
    if [ ! -f "${src_full}" ]; then
        echo "ERROR: Image not found → ${src_full}" >&2
        exit 1
    fi
    if [ ! -f "${pubkey_full}" ] || [ ! -f "${privkey_full}" ]; then
        echo "ERROR: Key files not found → pubkey: ${pubkey_full} privkey: ${privkey_full}" >&2
        exit 1
    fi

    # 3. 删除旧签名文件
    rm -rf "${output_full}"

    # 4. 选择签名命令（hash/hashtree）
    local sign_cmd=""
    if [ "${sign_type}" = "hash" ]; then
        sign_cmd="make_hash_footer"
    elif [ "${sign_type}" = "hashtree" ]; then
        sign_cmd="make_hashtree_footer"
    else
        echo "ERROR: Signature type not supported → ${sign_type} (Only hash/hashtree are supported." >&2
        exit 1
    fi

    # 5. 执行签名（换行格式化，便于核对参数）
    echo "INFO: start ${sign_type} signature → ${src_img}"
    $CMD_SIGN "${sign_cmd}" \
        --image "${src_full}" \
        --partition "${partition}" \
        --partition_size "${part_size}" \
        --pubkey "${pubkey_full}" \
        --privkey "${privkey_full}" \
        --algorithm "${ALGORITHM}" \
        --rollback_index "${rollback_ind}" \
        --rollback_location "${rollback_loc}" \
        --output "${output_full}"

    # 6. 验证签名结果
    if [ $? -eq 0 ]; then
        echo "SUCCESS: ${src_img} ${sign_type} signature succeeded → ${output_full}"
    else
        echo "ERROR: ${src_img} ${sign_type} signature failed!" >&2
        exit 1
    fi
}

# hash签名 - dtbo.img
sign_image \
    "hash" \
    "dtbo.img" \
    "dtbo_a" \
    "8388608" \
    "rsa2048_public_boot.pem" \
    "rsa2048_private_boot.pem" \
    "${ROLLBACK_INDEX}" \
    "1" \
    "signed_dtbo.img"

# hashtree签名 - boot.img
sign_image \
    "hashtree" \
    "boot.img" \
    "boot_a" \
    "67108864" \
    "rsa2048_public_boot.pem" \
    "rsa2048_private_boot.pem" \
    "${ROLLBACK_INDEX}" \
    "4" \
    "signed_boot.img"

# hashtree签名 - vendor_boot.img / vendor_boot_sec.img（根据kernel路径选择）
if [ -d "${KERNEL_PATH}" ]; then
    sign_image \
        "hashtree" \
        "vendor_boot.img" \
        "vendor_boot_a" \
        "104857600" \
        "rsa2048_public_vendor_boot.pem" \
        "rsa2048_private_vendor_boot.pem" \
        "${ROLLBACK_INDEX}" \
        "5" \
        "signed_vendor_boot.img"
else
    sign_image \
        "hashtree" \
        "vendor_boot_sec.img" \
        "vendor_boot_a" \
        "104857600" \
        "rsa2048_public_vendor_boot.pem" \
        "rsa2048_private_vendor_boot.pem" \
        "${ROLLBACK_INDEX}" \
        "5" \
        "signed_vendor_boot.img"
fi

# hashtree签名 - system.img
sign_image \
    "hashtree" \
    "system.img" \
    "system" \
    "3145728000" \
    "rsa2048_public_system.pem" \
    "rsa2048_private_system.pem" \
    "${ROLLBACK_INDEX}" \
    "6" \
    "signed_system.img"

# hashtree签名 - vendor.img
sign_image \
    "hashtree" \
    "vendor.img" \
    "vendor" \
    "838860800" \
    "rsa2048_public_vendor.pem" \
    "rsa2048_private_vendor.pem" \
    "${ROLLBACK_INDEX}" \
    "7" \
    "signed_vendor.img"


#rvt生成
rm -rf ${IMG_PATH}/rvt.img
${CMD_SIGN} \
    make_rvt_image \
    --pubkey ${KEY_PATH}/rsa2048_public.pem \
    --privkey ${KEY_PATH}/rsa2048_private.pem \
    --partition rvt \
    --partition_size 1048576 \
    --algorithm ${ALGORITHM} \
    --rollback_index ${ROLLBACK_INDEX} \
    --rollback_location 0 \
    --chain_partition boot_a:${KEY_PATH}/rsa2048_public_boot.pem \
    --chain_partition vendor_boot_a:${KEY_PATH}/rsa2048_public_vendor_boot.pem \
    --chain_partition dtbo_a:${KEY_PATH}/rsa2048_public_boot.pem  \
    --chain_partition system:${KEY_PATH}/rsa2048_public_system.pem \
    --chain_partition vendor:${KEY_PATH}/rsa2048_public_vendor.pem \
    --output ${IMG_PATH}/rvt.img

if [ $? -eq 0 ]; then
    echo "SUCCESS: rvt image generation succeeded"
else
    echo "ERROR: rvt image generation failed!" >&2
    exit 1
fi
