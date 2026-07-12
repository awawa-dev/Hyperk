import os
import json
import re
import shutil
Import("env")

effects_src_dir = env.subst("$PROJECT_DIR/effects")
template_src_file = env.subst("$PROJECT_DIR/include/uni_json_api_oss.h")

build_dir = env.subst("$BUILD_DIR")
gen_dir = os.path.join(build_dir, "generated_files")
gen_effects_dir = os.path.join(gen_dir, "effects")
output_factory_file = os.path.join(gen_dir, "effects_factory.h")

def clean_generated_dir():
    if os.path.exists(gen_effects_dir):
        shutil.rmtree(gen_effects_dir)
    os.makedirs(gen_effects_dir, exist_ok=True)

def make_safe_name(folder_name):
    safe = re.sub(r'[^a-zA-Z0-9]', '_', folder_name)
    if safe[0].isdigit():
        safe = "E_" + safe
    return safe[:20]

def generate_system():
    if not os.path.exists(effects_src_dir):
        print(f"[\033[91mERROR\033[0m] Directory {effects_src_dir} not found!")
        return

    clean_generated_dir()
    
    effects_json_list = []
    registry_includes = []
    registry_entries = []

    folders = [f for f in os.listdir(effects_src_dir) if os.path.isdir(os.path.join(effects_src_dir, f))]
    if "Solid" not in folders:
        print("[\033[91mERROR\033[0m] Required effect 'Solid' is missing in src/effects!")
        env.Exit(1)

    folders.sort(key=lambda name: (name != "Solid", name))

    for folder_name in folders:
        src_folder_path = os.path.join(effects_src_dir, folder_name)
        
        if not os.path.isdir(src_folder_path):
            continue

        safe_name = make_safe_name(folder_name)
        class_name = f"Effect_{safe_name}"
        
        src_effect_h = os.path.join(src_folder_path, "effect.h")
        if not os.path.exists(src_effect_h):
            print(f"[\033[93mWARNING\033[0m] No effect.h found in {folder_name}. Skipping.")
            continue

        effects_json_list.append(safe_name)

        dest_folder_path = os.path.join(gen_effects_dir, safe_name)
        os.makedirs(dest_folder_path, exist_ok=True)
        dest_effect_h = os.path.join(dest_folder_path, "effect.h")

        with open(src_effect_h, "r", encoding="utf-8") as f:
            content = f.read()

        new_content = re.sub(r'\bEFFECT_NAMESPACE\b', class_name, content)

        with open(dest_effect_h, "w", encoding="utf-8") as f:
            f.write(new_content)

        registry_includes.append(f'#include "effects/{safe_name}/effect.h"')
        registry_entries.append(f'        {{"{safe_name}", &{class_name}::run }}')
        
        print(f"[\033[92mSUCCESS\033[0m] Processed effect: {folder_name} -> {class_name}")

    # ==========================================
    json_str = json.dumps(effects_json_list)
    
    tpl_macro_content = ""
    if os.path.exists(template_src_file):
        with open(template_src_file, "r", encoding="utf-8") as f:
            tpl_content = f.read()
        
        match = re.search(r'#define\s+TPL_BODY\b.*?}\)raw"', tpl_content, flags=re.DOTALL)
        
        if match:
            extracted_macro = match.group(0)
            extracted_macro = re.sub(r'"effects":\[.*?\]', f'"effects":{json_str}', extracted_macro, flags=re.DOTALL)
            extracted_macro = re.sub(r'#define\s+TPL_BODY\b', '#define TPL_BODY_WITH_EFFECTS', extracted_macro)
            tpl_macro_content = extracted_macro
        else:
            print(f"[\033[91mERROR\033[0m] Could not find TPL_BODY definition in {template_src_file}!")
    else:
        print(f"[\033[93mWARNING\033[0m] {template_src_file} not found. Skipping JSON injection.")


    cpp_factory_code = f"""#pragma once
#include <array>
#include <utility>
#include <string_view>
#include "effects.h"

{tpl_macro_content}

{chr(10).join(registry_includes)}

using EffectRunFn = bool (*)(const EffectConfig& effectConfig, EffectSetup &effectSetup, EffectRun& effectRun);
using EffectPair = std::pair<const char*, EffectRunFn>;

constexpr std::size_t NUM_EFFECTS = {len(effects_json_list)};

inline const std::array<EffectPair, NUM_EFFECTS>& getEffectsRegistry() {{
    static const std::array<EffectPair, NUM_EFFECTS> registry = {{{{
{chr(10).join(registry_entries)}
    }}}};
    return registry;
}}
"""
    with open(output_factory_file, "w", encoding="utf-8") as f:
        f.write(cpp_factory_code)

    print(f"[\033[92mSUCCESS\033[0m] Generated {output_factory_file}")
    print(f"[\033[96mINFO\033[0m] Injected effects JSON: {json_str}")

# Start
generate_system()
