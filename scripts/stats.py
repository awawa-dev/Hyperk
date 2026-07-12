import os
import subprocess
from os.path import exists
from pathlib import Path

Import("env")
def dump_symbols(target, source, env):
    elf = os.path.join(
        env.subst("$BUILD_DIR"),
        env.subst("${PROGNAME}.elf")
    )

    cc = Path(env.subst("$CC"))
    nm = cc.with_name(cc.name.replace("-gcc", "-nm"))    
    
    print("ELF:", elf)
    print("NM :", nm)

    result = subprocess.run(
        [nm, "-SC", "--size-sort", elf],
        capture_output=True,
        text=True
    )

    print("\n>>> BIG RAM SYMBOLS")

    count = 0

    for line in reversed(result.stdout.splitlines()):
        parts = line.split()

        if len(parts) < 4:
            continue

        if parts[2] in ("B", "b", "D", "d"):
            print(line)
            count += 1

        if count >= 50:
            break
env.AddPostAction("buildprog", dump_symbols)