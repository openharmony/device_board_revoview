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

pushd ${1}
ROOT_BUILD_DIR=${3}
ROOT_DIR=${ROOT_BUILD_DIR}/../..
export PRODUCT_PATH=vendor/revoview/${2}
export TARGET_BUILD_VARIANT=${4}

KERNEL_SRC_TMP_PATH=${ROOT_BUILD_DIR}/kernel/src_tmp/linux-6.6
KERNEL_SOURCE=${ROOT_DIR}/kernel/linux/linux-6.6
KERNEL_PATCH_PATH=${ROOT_DIR}/kernel/linux/patches/linux-6.6
KERNEL_PATCH=${ROOT_DIR}/kernel/linux/patches/linux-6.6/p7885_patch/kernel.patch
HDF_PATCH=${ROOT_DIR}/device/board/revoview/${2}/kernel
FIRMWARE_PATCH=${ROOT_DIR}/device/board/revoview/${2}/kernel
HCS_HCB=${ROOT_DIR}/${PRODUCT_PATH}/hdf_config/khdf/hdf_hcs.hcb

NEWIP_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/newip/apply_newip.sh
TZDRIVER_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/tzdriver/apply_tzdriver.sh
CODESIGN_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/code_sign/apply_code_sign.sh
XPM_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/xpm/apply_xpm.sh
CED_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/container_escape_detection/apply_ced.sh
HIDEADDR_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/memory_security/apply_hideaddr.sh
QOS_AUTH_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/qos_auth/apply_qos_auth.sh
UNIFIED_COLLECTION_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/ucollection/apply_ucollection.sh
DEC_PATCH_FILE=${ROOT_DIR}/kernel/linux/common_modules/dec/apply_dec.sh

PRODUCT_DEFCOFIG=${KERNEL_SOURCE}/arch/arm64/configs/wukong100_defconfig




if [ -f ${HCS_HCB} ]
then
rm ${HCS_HCB}
fi

rm -rf ${KERNEL_SRC_TMP_PATH}
mkdir -p ${KERNEL_SRC_TMP_PATH}

cp -arf ${KERNEL_SOURCE}/* ${KERNEL_SRC_TMP_PATH}/
cp -arf ${FIRMWARE_PATCH}/firmware ${KERNEL_SRC_TMP_PATH}/

cd ${KERNEL_SRC_TMP_PATH}
#
#cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/audio ${KERNEL_SRC_TMP_PATH}/drivers/unisoc_platform/
cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/wukong100_defconfig ${KERNEL_SRC_TMP_PATH}/arch/arm64/configs/

cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/mkboot ${KERNEL_SRC_TMP_PATH}/.
cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/make_ohos.sh ${KERNEL_SRC_TMP_PATH}/.
cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/make_boot.sh ${KERNEL_SRC_TMP_PATH}/.
cp -rf ${ROOT_DIR}/device/board/revoview/${2}/kernel/make-build.sh ${KERNEL_SRC_TMP_PATH}/.

mkdir -p ${KERNEL_SRC_TMP_PATH}/mkboot/dist

#HDF patch
bash ${ROOT_DIR}/drivers/hdf_core/adapter/khdf/linux/patch_hdf.sh ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${KERNEL_PATCH_PATH} wukong100

    #update linux-6.6 kernel stdarg.h path to linux/stdarg.h
          sed -i 's/<stdarg.h>/<linux\/stdarg.h>/' ${KERNEL_SRC_TMP_PATH}/bounds_checking_function/include/securec.h


#dec
if [ -f $DEC_PATCH_FILE ]; then
    bash $DEC_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi

#kernel patch
    patch -p1 < ${KERNEL_PATCH}

#newip
if [ -f $NEWIP_PATCH_FILE ]; then
    bash $NEWIP_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi

#tzdriver
if [ -f $TZDRIVER_PATCH_FILE ]; then
    bash $TZDRIVER_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi

#code sign
if [ -f $CODESIGN_PATCH_FILE ]; then
    bash $CODESIGN_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi

#xpm
if [ -f $XPM_PATCH_FILE ]; then
    bash $XPM_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi
    #ced
    if [ -f $CED_PATCH_FILE ]; then
        bash $CED_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
    fi

#qos_auth
if [ -f $QOS_AUTH_PATCH_FILE ]; then
    bash $QOS_AUTH_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi

    #hideaddr
    if [ -f $HIDEADDR_PATCH_FILE ]; then
        bash $HIDEADDR_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
    fi

#ucollection
if [ -f $UNIFIED_COLLECTION_PATCH_FILE ]; then
    bash $UNIFIED_COLLECTION_PATCH_FILE ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${DEVICE_NAME} linux-6.6
fi
#selinux config patch
#for arg in "$@"; do
#    if [ "$arg" = "is_release" ]; then
#        echo "close selinux kernel config CONFIG_SECURITY_SELINUX_DEVELOP in release version"
#        ${KERNEL_SOURCE}/scripts/config --file ${KERNEL_SRC_TMP_PATH}/arch/arm64/configs/wukong100_defconfig -d SECURITY_SELINUX_DEVELOP
#    fi
#done


#build kernel

./make_ohos.sh

popd
