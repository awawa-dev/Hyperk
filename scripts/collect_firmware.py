import os
import shutil
import re
from os.path import exists

Import("env")

def copy_firmware(target, source, env):
    env_name = env.get("PIOENV")
    app_version = "unknown"
    
    for flag in env.get("CPPDEFINES", []):
        key = flag[0] if isinstance(flag, tuple) else flag
        if key == "APP_VERSION":
            val = str(flag[1]) if isinstance(flag, tuple) else ""
            app_version = re.sub(r'[^a-zA-Z0-9.-]', '', val)
            break

    project_dir = env.get("PROJECT_DIR")
    dist_dir = os.path.join(project_dir, "release")
    
    if not os.path.exists(dist_dir):
        os.makedirs(dist_dir)


    exts = [".uf2"] if "pico" in env_name else [".bin"]
    base_path = env.subst("$BUILD_DIR/${PROGNAME}")

    for ext in exts:
        source_path = base_path + ext
        
        if os.path.exists(source_path):
            dest_name = f"Hyperk_{app_version}_{env_name}{ext}"
            dest_path = os.path.join(dist_dir, dest_name)
            
            print(f">>> [COPY] {os.path.basename(source_path)} -> {dest_path}")
            shutil.copy2(source_path, dest_path)
        else:
            print(f">>> [ERROR] File not found: {source_path}")
            
env.AddPostAction("buildprog", copy_firmware)