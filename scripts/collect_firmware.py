import os
import shutil
import re
import subprocess
from os.path import exists

Import("env")

def run_merge_bin(mcu, factory_path, boot_addr, bootloader, partitions, source_path, env):
    cmd_name = "merge_bin"
    f_mode_flag = "--flash_mode"
    f_size_flag = "--flash_size"

    cmd = [
        env.subst("$PYTHONEXE"), "-m", "esptool",
        "--chip", mcu,
        cmd_name,
        "-o", factory_path,
        f_mode_flag, env.get("BOARD_FLASH_MODE", "dio"),
        f_size_flag, env.get("BOARD_FLASH_SIZE", "4MB"),
        boot_addr, bootloader,
        "0x8000", partitions,
        "0x10000", source_path
    ]
    
    subprocess.run(cmd, check=True)

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

    build_dir = env.subst("$BUILD_DIR")
    is_pico = "pico" in env_name
    ext = ".uf2" if is_pico else ".bin"
    source_path = os.path.join(build_dir, env.subst("${PROGNAME}") + ext)

    if os.path.exists(source_path):
        # 1. Kopiowanie OTA
        dest_name = f"Hyperk_{app_version}_{env_name}{ext}"
        dest_path = os.path.join(dist_dir, dest_name)
        shutil.copy2(source_path, dest_path)
        print(f">>> [COPY] {dest_name}")

        # 2. Factory Merge
        if ext == ".bin" and "esp" in env_name:            
            bootloader = os.path.join(build_dir, "bootloader.bin")
            partitions = os.path.join(build_dir, "partitions.bin")
            
            if os.path.exists(bootloader) and os.path.exists(partitions):
                mcu = env.get("BOARD_MCU", "esp32")
                boot_addr = "0x1000" if mcu == "esp32" else "0x0"
                factory_path = os.path.join(dist_dir, f"Hyperk_{app_version}_{env_name}_factory_flash.bin")
                try:
                    run_merge_bin(mcu, factory_path, boot_addr, bootloader, partitions, source_path, env)
                    print(f">>> [SUCCESS] Factory image: {os.path.basename(factory_path)}")
                except Exception as e:
                    print(f">>> [ERROR] Merge failed for {env_name}: {e}")
                    env.Exit(1)
    else:
        print(f">>> [ERROR] File not found: {source_path}")

env.AddPostAction("buildprog", copy_firmware)