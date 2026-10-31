"""Optional: fetch a pinned historical MAME 2000 libretro source checkout.
Does not build or link a core, never writes outside sample/mame/external.
No ROMs or BIOS are downloaded.
"""
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
DEST = HERE / 'external' / 'mame2000-libretro'
URL = 'https://github.com/libretro/mame2000-libretro.git'
REV = '181b42e5da0d2f0c7a2e59244f23633ed594de2e'

def git(*args, cwd=None):
    return subprocess.check_output(['git', *args], cwd=cwd, text=True).strip()

if DEST.exists():
    if not (DEST / '.git').is_dir():
        raise SystemExit(f'FAIL: {DEST} exists but is not a git checkout; no overwrite')
    if git('status', '--porcelain', cwd=DEST):
        raise SystemExit('FAIL: upstream checkout has modifications; no overwrite')
    remote = git('remote', 'get-url', 'origin', cwd=DEST)
    if remote != URL:
        raise SystemExit('FAIL: upstream remote differs; no overwrite')
else:
    DEST.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['git', 'clone', '--filter=blob:none', URL, str(DEST)], check=True)
subprocess.run(['git', 'fetch', 'origin', REV], cwd=DEST, check=True)
subprocess.run(['git', 'checkout', '--detach', REV], cwd=DEST, check=True)
if git('rev-parse', 'HEAD', cwd=DEST) != REV:
    raise SystemExit('FAIL: MAME source revision mismatch')
print(f'OK: pinned MAME2000 source at {DEST}\nNot an ESP32-P4 port; no source was added to the build.')
