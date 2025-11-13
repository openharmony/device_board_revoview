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


DIR=$(cd $(dirname $0) && pwd)
echo $DIR
echo kernel TARGET_BUILD_VARIANT is $TARGET_BUILD_VARIANT

export PATH=$DIR/../../../../../prebuilts/clang/ohos/linux-x86_64/llvm/bin/:$DIR/../../../../../prebuilts/develop_tools/pahole/bin/:$PATH
export PRODUCT_PATH=vendor/revoview/wukong100 
export KBUILD_OUTPUT=../../OBJ/linux-5.15

CPUs=`sed -n "N;/processor/p" /proc/cpuinfo|wc -l`
MAKE="make LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu-"
#CROSS_COMPILE=../../../../../prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
#CROSS_COMPILE=../../../../../prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
#CROSS_COMPILE_COMPAT=../../../../../prebuilts/gcc/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/bin/arm-linux-gnueabi-

${MAKE} ARCH=arm64 LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu- wukong100_defconfig
${MAKE} ARCH=arm64 LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu- -j$CPUs

cp -f ${KBUILD_OUTPUT}/arch/arm64/boot/Image mkboot/dist/Image
cp -f ${KBUILD_OUTPUT}/arch/arm64/boot/dts/sprd/*.dtb mkboot/dist
cp -f ${KBUILD_OUTPUT}/arch/arm64/boot/dts/sprd/uis7885-2h10-overlay.dtbo mkboot/dist/uis7885-2h10-overlay.dtbo
