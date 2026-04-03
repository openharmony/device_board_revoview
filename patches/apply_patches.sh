#  Copyright (c) 2025 Huawei Device Co., Ltd.
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash
set -e
# 该脚本在openharmony项目目录下预编译阶段执行。
OHOS_PROJECT_PATH=`pwd`
# echo "OHOS_PROJECT_PATH=" $OHOS_PROJECT_PATH

# 获取工具仓路径
PATCH_TOOL_PATH=$(cd `dirname $0`;pwd)
# echo "PATCH_TOOL_PATH =" $PATCH_TOOL_PATH


# 门禁在预编译阶段sig项目和openharmony项目还未合并
SIG_PROJECT_PATH=$(cd `dirname $OHOS_PROJECT_PATH`;pwd)/$2
# echo "SIG_PROJECT_PATH=" $SIG_PROJECT_PATH

#获取补丁配置目录patchs
PATCHES_PATH=""
xml=$1
xml_path=$SIG_PROJECT_PATH/.repo/manifests/$xml
fist_project=`cat $xml_path | grep "<project" | head -n 1`

fist_repo_path=${fist_project#*path=\"}

# 路径与仓名一致时可不写path字段
if [ "$fist_repo_path" == "$fist_project" ];then
    fist_repo_name=${fist_project#*name=\"}
    fist_repo_name=${fist_repo_name%%\"*}
    PATCHES_PATH=$SIG_PROJECT_PATH/$fist_repo_name/patches
else
    fist_repo_path=${fist_repo_path%%\"*}
    PATCHES_PATH=$SIG_PROJECT_PATH/$fist_repo_path/patches
fi
cd $OHOS_PROJECT_PATH

# 判断是否有补丁配置文件
if [ -f "${PATCHES_PATH}/patches.json" ];then
echo "PATCHES_PATH =" $PATCHES_PATH
echo "PATCH_TOOL_PATH =" $PATCH_TOOL_PATH
    python3  $PATCH_TOOL_PATH/apply_patches.py $OHOS_PROJECT_PATH $PATCHES_PATH
fi

