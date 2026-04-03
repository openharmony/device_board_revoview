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

import json
import os
import sys
import subprocess
import xml.etree.ElementTree as ET

#从流水线环境中解析patches.json内容。
def parse_patches_file(ohos_root_path,manifest_name,sig_dirname):
    sig_root_path= os.path.join(os.path.dirname(ohos_root_path),sig_dirname)
    print("bychris sig_root_path: {} !!".format(sig_root_path))
    manifest_path=os.path.join(sig_root_path,".repo/manifests",manifest_name)
    # 获取sig项目manifest.xml中的第一个仓
    first_sig_project = read_first_project_from_xml(manifest_path)
    patches_file = os.path.join(sig_root_path,first_sig_project["path"],"patches/patches.json")
    if not os.path.exists(patches_file):
        print("there is no {} file !!! do not need co-build!!!!".format(os.path.join(first_sig_project["path"],"patches/patches.json")))
        sys.exit(-1)
    patches_data=read_json_file(patches_file)
    return patches_data
    


def apply_prs(ohos_root_path, patches_data):
    # Iterate over patches and apply them to the corresponding projects
    is_success=True
    for patch_info in patches_data['patches']:
        project_name = patch_info['project']
        project_dir = os.path.join(ohos_root_path,patch_info['path'])
        if 'pr_url' in patch_info:
            pr_url = patch_info['pr_url']
            print("Start get fetch pr {}".format(pr_url))
            result = fetch_pr(project_dir,project_name,pr_url)
            print("bychris {}.".format(result[1]))
            if result[0] != 0:
                is_success=False
        else:
            print("bychris 没有PR URL")

    if not is_success:
        sys.exit(-1)

def fetch_pr(pr_repo_path,pr_repo_name,pr_url):
    pr_number = pr_url.split("/")[-1]
    fetch_cmd = "timeout 600s git fetch -f https://gitcode.com/openharmony/{0}.git merge-requests/{1}/head:pr_{1}".format(pr_repo_name,pr_number)
    fetch_output = run_shell_cmd(fetch_cmd,pr_repo_path)
    if fetch_output[0] != 0:
        return fetch_output
    merge_cmd = "timeout 600s git merge pr_{} --no-edit".format(pr_number)
    merge_output = run_shell_cmd(merge_cmd,pr_repo_path)
    if merge_output[0] != 0:
        return merge_output
    confirm_status_cmd="git status -s|grep -v \"^??\""
    confirm_status_output =run_shell_cmd(confirm_status_cmd,pr_repo_path)
    if confirm_status_output[1]:
        return (1,"git fetch pr failed: {}".format(confirm_status_output[1]))
    return (0,"git fetch pr {} success!\n".format(pr_url))

def run_shell_cmd(cmd,pwd=""):
    if pwd and not os.path.exists(pwd):
        return (1,"No such file or directory: %s"%pwd)
    elif pwd:
        os.chdir(pwd)
    result = subprocess.getstatusoutput(cmd)
    return result

def read_json_file(input_file):
    if not os.path.exists(input_file):
        print("file '{}' doesn't exist.".format(input_file))
        return None

    data = None
    try:
        with open(input_file, 'r') as input_f:
            data = json.load(input_f)
    except json.decoder.JSONDecodeError:
        print("The file '{}' format is incorrect.".format(input_file))
        raise
    return data

def read_first_project_from_xml(input_file):
    tree = ET.parse(input_file)
    return tree.findall("project")[0].attrib
        


if __name__ == "__main__":
    ohos_root_path = os.getcwd()
    # sig工程使用的manifest.xml文件
    manifest_name = sys.argv[1]
    # sig工程的根目录名称
    sig_dirname = sys.argv[2]

    print("bychris ohos_root_path: {} !!".format(ohos_root_path))
    #get patches info
    patches_data = parse_patches_file(ohos_root_path,manifest_name,sig_dirname)
    # Apply prs to the mainline repositories
    apply_prs(ohos_root_path,patches_data)
