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

# Ensure subdirectories
win_vst3_dst = os.path.join(dist_dir, "Windows", "VST3", "VanceSpectral.vst3")
win_clap_dst = os.path.join(dist_dir, "Windows", "CLAP")
win_aax_dst = os.path.join(dist_dir, "Windows", "AAX", "VanceSpectral.aaxplugin")

mac_vst3_dst = os.path.join(dist_dir, "macOS", "VST3")
mac_clap_dst = os.path.join(dist_dir, "macOS", "CLAP")
mac_aax_dst = os.path.join(dist_dir, "macOS", "AAX")
mac_au_dst = os.path.join(dist_dir, "macOS", "AU")

for d in [os.path.dirname(win_vst3_dst), win_clap_dst, os.path.dirname(win_aax_dst), mac_vst3_dst, mac_clap_dst, mac_aax_dst, mac_au_dst]:
    os.makedirs(d, exist_ok=True)

# Copy Release VST3
win_vst3_src = os.path.join(release_dir, "VST3", "VanceSpectral.vst3")
if os.path.exists(win_vst3_dst):
    shutil.rmtree(win_vst3_dst, onerror=remove_readonly)
if os.path.exists(win_vst3_src):
    shutil.copytree(win_vst3_src, win_vst3_dst)
    print(f"[+] Windows VST3 copied from {win_vst3_src}")

# Copy Release AAX
win_aax_src = os.path.join(release_dir, "AAX", "VanceSpectral.aaxplugin")
if os.path.exists(win_aax_dst):
    shutil.rmtree(win_aax_dst, onerror=remove_readonly)
if os.path.exists(win_aax_src):
    shutil.copytree(win_aax_src, win_aax_dst)
    print(f"[+] Windows AAX copied from {win_aax_src}")

# Verify README.txt
readme_path = os.path.join(dist_dir, "README.txt")
if os.path.exists(readme_path):
    print("[+] README.txt verified")

# Zip VanceSpectral-Plugin
if os.path.exists(zip_path):
    os.remove(zip_path)

print("[+] Creating VanceSpectral-Plugin.zip...")
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(dist_dir):
        for file in files:
            full_path = os.path.join(root, file)
            rel_path = os.path.relpath(full_path, repo_root)
            zipf.write(full_path, rel_path)

print(f"[SUCCESS] VanceSpectral-Plugin folder and {zip_path} created successfully!")
