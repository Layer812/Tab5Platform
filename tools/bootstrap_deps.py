"""Acquire exactly the X68KTab5 R6-validated M5 libs and overlay scripts.
Does not access or modify the X68KTab5 checkout or the connected Tab5.
Requires git, Python 3 and network access on the developer's Windows PC.
"""
from pathlib import Path
import subprocess
import urllib.request
import sys

ROOT = Path(__file__).resolve().parents[1]
R6 = "802a10bf915d221ae91c950029d5e812ad4d085a"
PIN = {
    "M5GFX": ("https://github.com/m5stack/M5GFX.git", "65c816a38386062d2c3dc47d41237a9083acbc75"),
    "M5Unified": ("https://github.com/m5stack/M5Unified.git", "368ad60807161625aa65baad37a4a3af2a7eccea"),
}
SCRIPTS = ["patch_m5gfx_x68ktab.py", "patch_m5unified_x68ktab_es8388.py", "patch_m5unified_x68ktab_speaker_idle.py"]

def run(*args, cwd=None):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(args, cwd=cwd or ROOT, check=True)

for name, (url, sha) in PIN.items():
    path = ROOT / "components" / name
    if not (path / ".git").exists():
        if path.exists() and any(path.iterdir()):
            raise SystemExit(f"STOP: existing non-git component {path}; no overwrite")
        run("git", "clone", "--filter=blob:none", url, str(path))
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=path, text=True).strip()
    if head != sha:
        run("git", "fetch", "origin", sha, cwd=path)
        status = subprocess.check_output(["git", "status", "--porcelain"], cwd=path, text=True)
        if status.strip():
            raise SystemExit(f"STOP: modified {name}; refusing checkout of pinned R6 version")
        run("git", "checkout", "--detach", sha, cwd=path)
    print(f"Pinned {name}: {sha}")

for file in SCRIPTS:
    dest=ROOT/file
    if not dest.exists():
        url=f"https://raw.githubusercontent.com/Layer812/X68KTab5/{R6}/{file}"
        print("Fetch R6 overlay:", url, flush=True)
        with urllib.request.urlopen(url, timeout=45) as response:
            content=response.read()
        if not content or b"#!/" not in content[:128] and b"import " not in content[:512]:
            raise SystemExit(f"STOP: unexpected overlay content {file}")
        dest.write_bytes(content)
    if file == "patch_m5unified_x68ktab_speaker_idle.py":
        # R6 patch is an LF multiline raw-byte matcher. Windows Git can
        # check out the pinned Speaker_Class.inl as CRLF. Preserve exact R6
        # patch contents while accepting either newline encoding.
        run(sys.executable, str(ROOT / "tools" / "apply_r6_speaker_overlay.py"), cwd=ROOT)
    else:
        run(sys.executable, str(dest), cwd=ROOT)
print("READY: dependencies and R6 overlays applied to Tab5Platform copy")
