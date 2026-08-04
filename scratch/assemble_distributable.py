import os
import shutil
import stat
import zipfile

def remove_readonly(func, path, excinfo):
    os.chmod(path, stat.S_IWRITE)
    func(path)

repo_root = r"c:\Users\Ansh Srivastava\OneDrive\Documents\GitHub\VanceSpectral"
dist_dir = os.path.join(repo_root, "VanceSpectral-Plugin")
zip_path = os.path.join(repo_root, "VanceSpectral-Plugin.zip")
release_dir = os.path.join(repo_root, "Builds", "VisualStudio2022", "x64", "Release")

# Ensure base folders exist
os.makedirs(dist_dir, exist_ok=True)

# Define folder structure
windows_vst3 = os.path.join(dist_dir, "Windows", "VST3", "VanceSpectral.vst3")
windows_clap = os.path.join(dist_dir, "Windows", "CLAP")
windows_aax = os.path.join(dist_dir, "Windows", "AAX", "VanceSpectral.aaxplugin")

macos_vst3 = os.path.join(dist_dir, "macOS", "VST3")
macos_clap = os.path.join(dist_dir, "macOS", "CLAP")
macos_aax = os.path.join(dist_dir, "macOS", "AAX")
macos_au = os.path.join(dist_dir, "macOS", "AU")

for d in [windows_clap, macos_vst3, macos_clap, macos_aax, macos_au]:
    os.makedirs(d, exist_ok=True)

# Clean existing Windows VST3 & AAX bundles
if os.path.exists(windows_vst3):
    shutil.rmtree(windows_vst3, onerror=remove_readonly)
if os.path.exists(windows_aax):
    shutil.rmtree(windows_aax, onerror=remove_readonly)

# Copy VST3 bundle
vst3_src_bin = os.path.join(release_dir, "VST3", "NewProject.vst3", "Contents", "x86_64-win", "NewProject.vst3")
vst3_src_json = os.path.join(release_dir, "VST3", "NewProject.vst3", "Contents", "Resources", "moduleinfo.json")

vst3_dst_bin_dir = os.path.join(windows_vst3, "Contents", "x86_64-win")
vst3_dst_res_dir = os.path.join(windows_vst3, "Contents", "Resources")
os.makedirs(vst3_dst_bin_dir, exist_ok=True)
os.makedirs(vst3_dst_res_dir, exist_ok=True)

if os.path.exists(vst3_src_bin):
    shutil.copy2(vst3_src_bin, os.path.join(vst3_dst_bin_dir, "VanceSpectral.vst3"))
    print("[+] VST3 binary copied and named VanceSpectral.vst3")

if os.path.exists(vst3_src_json):
    shutil.copy2(vst3_src_json, os.path.join(vst3_dst_res_dir, "moduleinfo.json"))
    print("[+] VST3 moduleinfo.json copied")

# Copy AAX bundle
aax_src_bin = os.path.join(release_dir, "AAX", "NewProject.aaxplugin", "Contents", "x64", "NewProject.aaxplugin")
aax_src_res = os.path.join(release_dir, "AAX", "NewProject.aaxplugin", "Contents", "Resources")

aax_dst_bin_dir = os.path.join(windows_aax, "Contents", "x64")
aax_dst_res_dir = os.path.join(windows_aax, "Contents", "Resources")
os.makedirs(aax_dst_bin_dir, exist_ok=True)
os.makedirs(aax_dst_res_dir, exist_ok=True)

if os.path.exists(aax_src_bin):
    shutil.copy2(aax_src_bin, os.path.join(aax_dst_bin_dir, "VanceSpectral.aaxplugin"))
    print("[+] AAX binary copied and named VanceSpectral.aaxplugin")

if os.path.exists(aax_src_res):
    for item in os.listdir(aax_src_res):
        s = os.path.join(aax_src_res, item)
        d = os.path.join(aax_dst_res_dir, item)
        if os.path.isfile(s):
            shutil.copy2(s, d)

# Zip VanceSpectral-Plugin directory
if os.path.exists(zip_path):
    os.remove(zip_path)

print("[+] Creating VanceSpectral-Plugin.zip archive...")
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(dist_dir):
        for file in files:
            full_path = os.path.join(root, file)
            rel_path = os.path.relpath(full_path, repo_root)
            zipf.write(full_path, rel_path)

print("[SUCCESS] VanceSpectral-Plugin folder & zip archive generated successfully!")
