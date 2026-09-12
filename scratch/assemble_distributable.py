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

print("[*] Assembling clean VanceSpectral-Plugin distribution folder...")

# Ensure base folders exist
os.makedirs(dist_dir, exist_ok=True)

# Define folder structure
windows_vst3_bundle = os.path.join(dist_dir, "Windows", "VST3", "VanceSpectral.vst3")
windows_clap = os.path.join(dist_dir, "Windows", "CLAP")
windows_aax_bundle = os.path.join(dist_dir, "Windows", "AAX", "VanceSpectral.aaxplugin")

macos_vst3 = os.path.join(dist_dir, "macOS", "VST3")
macos_clap = os.path.join(dist_dir, "macOS", "CLAP")
macos_aax = os.path.join(dist_dir, "macOS", "AAX")
macos_au = os.path.join(dist_dir, "macOS", "AU")

for d in [windows_clap, macos_vst3, macos_clap, macos_aax, macos_au]:
    os.makedirs(d, exist_ok=True)

# Clean existing Windows VST3 & AAX bundles
if os.path.exists(windows_vst3_bundle):
    shutil.rmtree(windows_vst3_bundle, onerror=remove_readonly)
if os.path.exists(windows_aax_bundle):
    shutil.rmtree(windows_aax_bundle, onerror=remove_readonly)

# 1. Copy VST3 bundle from Builds/VisualStudio2022/x64/Release/VST3/VanceSpectral.vst3
vst3_source_bundle = os.path.join(release_dir, "VST3", "VanceSpectral.vst3")
if os.path.exists(vst3_source_bundle):
    shutil.copytree(vst3_source_bundle, windows_vst3_bundle)
    print(f"[+] Copied fresh VST3 bundle: {windows_vst3_bundle}")
else:
    raise FileNotFoundError(f"Missing built VST3 bundle at: {vst3_source_bundle}")

# 2. Copy AAX bundle from Builds/VisualStudio2022/x64/Release/AAX/VanceSpectral.aaxplugin
aax_source_bundle = os.path.join(release_dir, "AAX", "VanceSpectral.aaxplugin")
if os.path.exists(aax_source_bundle):
    shutil.copytree(aax_source_bundle, windows_aax_bundle)
    print(f"[+] Copied fresh AAX bundle: {windows_aax_bundle}")
else:
    raise FileNotFoundError(f"Missing built AAX bundle at: {aax_source_bundle}")

# 3. Clean any stray build artifacts, debug symbols (.pdb), or temp files if present in dist_dir
unwanted_extensions = {".pdb", ".obj", ".lib", ".exp", ".iobj", ".ipdb", ".ilk", ".tlog"}
removed_count = 0
for root, dirs, files in os.walk(dist_dir):
    for file in files:
        ext = os.path.splitext(file)[1].lower()
        if ext in unwanted_extensions or file.startswith("."):
            fp = os.path.join(root, file)
            os.remove(fp)
            removed_count += 1
if removed_count > 0:
    print(f"[+] Purged {removed_count} intermediate/stray files from distribution folder.")

# 4. Create clean Zip archive
if os.path.exists(zip_path):
    os.remove(zip_path)

print("[*] Creating VanceSpectral-Plugin.zip archive...")
file_count = 0
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(dist_dir):
        # Archive directory entries so empty folders are preserved on extract
        for d in dirs:
            dir_full = os.path.join(root, d)
            rel_dir = os.path.relpath(dir_full, repo_root).replace('\\', '/') + '/'
            zipinfo = zipfile.ZipInfo(rel_dir)
            zipf.writestr(zipinfo, '')
        for file in files:
            full_path = os.path.join(root, file)
            rel_path = os.path.relpath(full_path, repo_root).replace('\\', '/')
            zipf.write(full_path, rel_path)
            file_count += 1

print(f"[SUCCESS] Packaging complete: {zip_path} ({file_count} files archived)")
