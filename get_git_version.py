import subprocess

Import("env")

def get_firmware_specifier_build_flag():
    #ret = subprocess.run(["git", "describe"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
    ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
    build_tag = ret.stdout.strip()
    
    ret = subprocess.run(["git", "rev-list", "--count", f"{build_tag}..HEAD"], stdout=subprocess.PIPE, text=True)
    build_delta = ret.stdout.strip()

    ret = subprocess.run(["git", "rev-parse", "--short", f"HEAD"], stdout=subprocess.PIPE, text=True)
    build_hash = ret.stdout.strip()

    build_version = f"{build_tag}-{build_delta}-g{build_hash}"
    build_flag = "-D AUTO_VERSION=\\\"" + build_version + "\\\""
    print ("Firmware Revision: " + build_version)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)