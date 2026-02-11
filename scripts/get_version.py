import os

Import("env")

version_file = os.path.join(env.get("PROJECT_DIR"), "version")

if os.path.exists(version_file):
    with open(version_file, "r") as f:
        version = f.read().strip()
else:
    version = "0.0.0-unknown"

print(f">>> Detected Project Version: {version}")

env.Append(CPPDEFINES=[
    ("APP_VERSION", f'\\"{version}\\"')
])

env.Replace(PROGNAME=f"Hyperk_{version}")