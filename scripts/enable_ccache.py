import os
import sys

Import("env")

pio_env = env.get("PIOENV", "default")
ccache_dir = os.path.join(env["PROJECT_DIR"], ".ccache", pio_env)
os.makedirs(ccache_dir, exist_ok=True)

os.environ["CCACHE_DIR"] = ccache_dir
env.Append(ENV={"CCACHE_DIR": ccache_dir})

if sys.platform.startswith("win32"):
    local = os.path.join(env["PROJECT_DIR"], "ccache.exe")
    ccache_cmd = f'"{local}"' if os.path.exists(local) else "ccache"
else:
    ccache_cmd = "ccache"

print(f"\n[ccache] === {pio_env.upper()} ===")
print(f"[ccache] DIR: {ccache_dir}")
print(f"[ccache] CMD: {ccache_cmd}")

for var in ("CCCOM", "CXXCOM", "ASCOM"):
    if var in env:
        old = env[var]
        env[var] = f"{ccache_cmd} {old}"
        print(f"[ccache] Wrapped {var} → {env[var]}")

os.environ.update({
    "CCACHE_SLOPPINESS": "pch_defines,time_macros,include_file_mtime,include_file_ctime,file_macro,locale",
    "CCACHE_COMPRESS": "1",
    "CCACHE_COMPRESSLEVEL": "6",
    "CCACHE_MAXSIZE": "200M",
    "CCACHE_BASEDIR": env["PROJECT_DIR"]
})
env.Append(ENV=dict(os.environ))
