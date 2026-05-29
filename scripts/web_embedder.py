import os
import gzip
import urllib.parse
from pathlib import Path
Import("env")

def embed_web_files(data_dir, output_file, sufix=""):
    if not os.path.exists(data_dir):
        print(f"[WebEmbedder] Error: {data_dir} directory not found!")
        return
    
    raw_v = "1.0.0" # fallback
    cpp_defines = env.get("CPPDEFINES", []) 
    for define in cpp_defines:
        if isinstance(define, tuple) and define[0] == "APP_VERSION":
            raw_v = define[1]
            break
        elif isinstance(define, str) and define.startswith("APP_VERSION="):
            raw_v = define.split('=')[1]
            break
    clean_v = raw_v.replace('\\"', '').strip('"')
    app_version = urllib.parse.quote(clean_v)

    print(f"[WebEmbedder] Generating {output_file} (version = {app_version})...")
    
    files_data = []

    Path(output_file).parent.mkdir(parents=True, exist_ok=True)
    
    with open(output_file, "w", encoding="utf-8") as f:
        # File header and guards
        f.write("// Generated file - do not edit\n")
        f.write(f"#ifndef WEB_RESOURCES{sufix}_H\n#define WEB_RESOURCES{sufix}_H\n\n#include <Arduino.h>\n\n")

        # === combine CSS do hyperk.css ===
        css_dir = os.path.join(data_dir, "css")
        combined_css = b""

        if not sufix:
            combined_css += f'@import url("/css/hyperk_OSS.css?v={app_version}");\n'.encode('utf-8')

        if os.path.exists(css_dir):            
            css_files_paths = []
            
            for c_root, _, c_files in os.walk(css_dir):
                for c_file in c_files:
                    if c_file.endswith(".css"):
                        css_files_paths.append(os.path.join(c_root, c_file))
            
            css_files_paths.sort()
            
            for css_path in css_files_paths:
                with open(css_path, "rb") as f_css:
                    combined_css += f_css.read() + b"\n"
        
        # 1. Generate byte arrays for each file
        currentPageIndex = 0
        pageHomeIndex = -1
        for root, dirs, files in os.walk(data_dir):
            for file in files:
                full_path = os.path.join(root, file)
                # Create a relative path and a valid C++ variable name
                rel_path = os.path.relpath(full_path, data_dir).replace("\\", "/")
                var_name = rel_path.replace(".", "_").replace("/", "_").replace("-", "_").upper()
                
                # Determine MIME type based on extension
                MIME_TYPES = {
                    ".html": "text/html",
                    ".css":  "text/css",
                    ".js":   "application/javascript",
                    ".json": "application/json",
                    ".png":  "image/png",
                    ".jpg":  "image/jpeg",
                    ".jpeg": "image/jpeg",
                    ".gif":  "image/gif",
                    ".svg":  "image/svg+xml",
                    ".ico":  "image/x-icon",
                    ".webp": "image/webp",
                    ".woff2": "font/woff2"
                }

                # Get extension and look up MIME type (default to "application/octet-stream")
                ext = os.path.splitext(file)[1].lower()
                mime = MIME_TYPES.get(ext, "application/octet-stream")
                
                with open(full_path, "rb") as f_in:
                    raw_data = f_in.read()

                    # replace version
                    if ext == ".html":
                        content = raw_data.decode("utf-8").replace("{{VERSION}}", app_version)
                        raw_data = content.encode("utf-8")

                    # Compress data with Gzip
                    compressed = b""
                    if ext == ".css":
                        if combined_css:
                            compressed = gzip.compress(combined_css)
                            var_name = var_name.replace(file.upper().replace(".", "_"), "HYPERK_CSS")
                            rel_path = rel_path.replace(file, f"hyperk{sufix}.css")
                            combined_css = b""
                        else:
                            continue
                    else:
                        compressed = gzip.compress(raw_data)
                    
                    f.write(f"const char PAGE{sufix}_{var_name}[] PROGMEM = {{ ")
                    f.write(", ".join([f"0x{b:02x}" for b in compressed]))
                    f.write(f" }};\n")
                    f.write(f"const uint32_t PAGE{sufix}_{var_name}_LEN = {len(compressed)};\n\n")
                    
                    files_data.append({
                        "url": "/" + rel_path,
                        "var": f"PAGE{sufix}_{var_name}",
                        "len": f"PAGE{sufix}_{var_name}_LEN",
                        "mime": mime
                    })
                    if file == "index.html":
                        pageHomeIndex = currentPageIndex                    
                    currentPageIndex = currentPageIndex + 1

        # 2. Define the Resource structure and the lookup table
        f.write(f"struct WebResource {{\n  const char* url;\n  PGM_P data;\n  uint32_t len;\n  const char* mime;\n}};\n\n")
        
        f.write(f"{'extern ' if sufix else ''}const WebResource webResources{sufix}[] = {{\n")
        for file in files_data:
            f.write(f"  {{ \"{file['url']}\", {file['var']}, {file['len']}, \"{file['mime']}\" }},\n")
        f.write("};\n\n")
        
        total_count = len(files_data)
        f.write(f"{'extern ' if sufix else ''}const uint16_t webResourcesCount{sufix} = {total_count};\n\n")

        if pageHomeIndex != -1:
            f.write(f"{'extern ' if sufix else ''}const uint16_t pageHomeIndex{sufix} = {pageHomeIndex};\n\n")

        f.write("#endif\n")
    print(f"[WebEmbedder] Successfully created {output_file}")

# Execute the embedding process immediately when the script is loaded by PlatformIO
generated_include_dir = os.path.join(env.subst("$BUILD_DIR"), "generated_files")
embed_web_files(os.path.join(env.get("PROJECT_SRC_DIR"), "backend", "data"), os.path.join(generated_include_dir, "web_resources.h"))
embed_web_files(os.path.join(env.get("PROJECT_DIR"), "data"), os.path.join(generated_include_dir, "web_resources_OSS.h"), "_OSS")
env.Append(CPPPATH=[generated_include_dir])  
