import os
import sys
from functools import partial
import urllib.request
import urllib.error
from pathlib import Path
Import("env")

pioenv = env.subst("$PIOENV")
project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")

version_file = os.path.join(project_dir, "version")
if not os.path.exists(version_file):
    print("\033[91mERROR: 'version' file is missing in the root directory!\033[0m")
    Exit(1)

with open(version_file, "r") as f:
    app_version = f.read().strip()

backend_src_dir = os.path.join(project_dir, "src", "backend")
libbackend_name = f"libBackend_{pioenv}.a"

# ==========================================
# MAIN SCRIPT DECISION LOGIC
# ==========================================

if os.path.exists(backend_src_dir):
    # ------------------------------------------
    # DEV MODE
    # ------------------------------------------
    print(f"--- DEV Mode (Author) | Version: {app_version} | Env: {pioenv} ---")
    
    env.Append(CCFLAGS=["-DIS_DEV_BUILD"])
    env.Append(CXXFLAGS=["-fno-exceptions", "-fno-rtti"])

    secret_include_dir = os.path.join(backend_src_dir, "include")
    env.Append(SRC_BUILD_FLAGS=["-Isrc/backend/include"])
    
    sys.path.append(os.path.join(os.getcwd(), backend_src_dir, "scripts"))
    sys.pycache_prefix = os.path.join( build_dir, "pycache")
    import create_backend_lib
    libBackendInvoker = partial(create_backend_lib.generate_secure_library, pioenv=pioenv, project_dir=project_dir, 
                                build_dir=build_dir, app_version=app_version, libbackend_name=libbackend_name)
    env.AddPostAction("buildprog", libBackendInvoker)

else:
    # ------------------------------------------
    # USER MODE
    # ------------------------------------------
    print(f"--- USER Mode (Open Source) | Version: {app_version} | Env: {pioenv} ---")

    env.Append(CXXFLAGS=["-fno-exceptions", "-fno-rtti"])
    
    cache_dir = os.path.join(project_dir, "export_backend", "precompiled", app_version, pioenv)
    os.makedirs(cache_dir, exist_ok=True)    
    local_lib_path = os.path.join(cache_dir, libbackend_name)

    if not os.path.exists(local_lib_path):        
        url = f"https://github.com/awawa-dev/Hyperk-Backend/releases/download/{app_version}/{libbackend_name}"        
        print(f"-> Downloading required backend v{app_version} from GitHub...")
        
        try:
            with urllib.request.urlopen(url, timeout=15) as response, open(local_lib_path, 'wb') as out_file:
                out_file.write(response.read())
            print(f"[\033[92mSUCCESS\033[0m] Successfully downloaded and cached.")
        except urllib.error.HTTPError as e:
            print(f"\033[91mERROR: Cannot find library (HTTP Code {e.code}).\033[0m")
            print(f"Probably version '{app_version}' for environment '{pioenv}' has not been released yet.")
            print(f"Checked URL: {url}")
            Exit(1)
        except Exception as e:
            print(f"\033[91mCONNECTION ERROR: Make sure you have internet access. ({e})\033[0m")
            Exit(1)
    else:
        print(f"-> Using precompiled backend v{app_version} from cache.")

    env.Append(LIBS=[env.File(local_lib_path)])

#//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
if env["PIOENV"] == "esp32s2":
    p = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32")) / "cores" / "esp32" / "esp32-hal-tinyusb.h"
    print("Patching:", p) 
    s = p.read_text(encoding="utf-8")
    old = "#include \"tusb_config.h\""
    new = "#if !defined(CFG_TUSB_CONFIG_FILE) || !defined(CONFIG_IDF_TARGET_ESP32S2)\n#include /*patched*/ \"tusb_config.h\"\n#endif"
    if old in s and new not in s:
        p.write_text(s.replace(old, new), encoding="utf-8")
        print("Patched:", p.name)
    elif new not in s:
        raise RuntimeError(f"Patch failed: expected pattern not found in {p}")        

    p = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32")) / "cores" / "esp32" / "esp32-hal-tinyusb.c"
    print("Patching:", p)
    s = p.read_text("utf-8")
    old = 'xTaskCreate(usb_device_task, "usbd", 4096, NULL, configMAX_PRIORITIES - 1, NULL);'
    new = '#if !defined(CONFIG_IDF_TARGET_ESP32S2)\nxTaskCreate(usb_device_task, "usbd", 4096, NULL, configMAX_PRIORITIES - 1, NULL)/*patched*/;\n#else\nxTaskCreate(usb_device_task, "usbd", CFG_TUD_MAINTASK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL)/*patched*/;\n#endif'
    if old in s:
        p.write_text(s.replace(old, new), "utf-8")   
        print("Patched:", p.name)
    elif new not in s:
        raise RuntimeError(f"Patch failed: expected pattern not found in {p}")
