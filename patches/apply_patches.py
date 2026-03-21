import json
import os
import sys
import subprocess


def apply_patches(ohos_root_path, patches_dir):
    patches_file = os.path.join(patches_dir,'patches.json')
    print("bychris patches_dir:'{}'".format(patches_dir))

    # Load patches JSON file
    patches_data = read_json_file(patches_file)

    # Iterate over patches and apply them to the corresponding projects
    is_success=True
    result_print=""
    for patch_info in patches_data['patches']:
        project_name = patch_info['project']
        project_path = patch_info['path']

        if 'patch_file' in patch_info:
            patch_file = patch_info['patch_file']
            patch_path = os.path.join(patches_dir, patch_file)
            project_dir = os.path.join(ohos_root_path,project_path)
            result = apply_patch(patch_path, project_dir)
            if result[0] != 0:
                is_success=False
                result_print += "\nStart patching!!! patche file: %s\n"%patch_file
                result_print+=result[1]
            # print("result:{}".format(result_print))
    if not is_success:
        sys.exit(-1)

def apply_patch(patch_path, project_dir):
    # Apply the patch using the 'patch' command or any other suitable method
    # This implementation assumes the 'patch' command is available
    command = f"patch -f --dry-run -p1 -d {project_dir} < {patch_path}"
    result = subprocess.getstatusoutput(command)
    if result[0] != 0:
        print("bychris try patch faild:{},{} ".format(result[0], patch_path))
    else:
        print("bychris try patch success:{}".format(patch_path))
        command = f"patch -f -p1 -d {project_dir} < {patch_path}"
        result = subprocess.getstatusoutput(command)
        print(f"bychris patch success result: {result[0]}")
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

if __name__ == "__main__":
    ohos_root_path = sys.argv[1]
    patchs_path = sys.argv[2]

    # Apply patches to the mainline repositories
    apply_patches(ohos_root_path,patchs_path)
