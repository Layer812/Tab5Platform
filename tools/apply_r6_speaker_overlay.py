#!/usr/bin/env python3
"""Apply the exact pinned X68KTab5 R6 Speaker fast-wake patch on LF or CRLF.

The patch data is taken from the downloaded, unchanged R6 patch file.
Only this Tab5Platform checkout is modified; the X68KTab5 checkout is untouched.
"""
from __future__ import annotations

import ast
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "components" / "M5Unified"
SOURCE = UPSTREAM / "src" / "utility" / "Speaker_Class.inl"
R6_OVERLAY = ROOT / "patch_m5unified_x68ktab_speaker_idle.py"
PINNED = "368ad60807161625aa65baad37a4a3af2a7eccea"
MARK = b"X68KTAB_SPK_FAST_WAKE_R1"


def blocks(patch_text: str) -> tuple[bytes, bytes]:
    result: dict[str, bytes] = {}
    for node in ast.parse(patch_text).body:
        if not isinstance(node, ast.Assign):
            continue
        for target in node.targets:
            if isinstance(target, ast.Name) and target.id in ("BEFORE", "AFTER"):
                value = ast.literal_eval(node.value)
                if not isinstance(value, bytes):
                    raise RuntimeError("Speaker patch literal is not bytes")
                result[target.id] = value
    if set(result) != {"BEFORE", "AFTER"}:
        raise RuntimeError("Pinned R6 patch is missing exact BEFORE/AFTER bytes")
    before, after = result["BEFORE"], result["AFTER"]
    if before.count(b"\n") < 8 or after.count(MARK) != 1:
        raise RuntimeError("Unexpected R6 Speaker patch block content")
    return before, after


def apply_source(src: bytes, before: bytes, after: bytes) -> tuple[bytes, str]:
    if b"\r\n" in src:
        if src.count(b"\r\n") != src.count(b"\n"):
            raise RuntimeError("Mixed line endings: refusing speaker source modification")
        before = before.replace(b"\n", b"\r\n")
        after = after.replace(b"\n", b"\r\n")
        style = "CRLF"
    else:
        style = "LF"
    if (b"if (flg_nodata)" not in src
            or b"i2s_config.clk_cfg.sample_rate_hz = _cfg.sample_rate;" not in src):
        raise RuntimeError("M5Unified Speaker source layout is unfamiliar")
    n_old, n_new = src.count(before), src.count(after)
    if n_old == 0 and n_new == 1:
        return src, style + " (already applied)"
    if n_old != 1 or n_new != 0:
        raise RuntimeError(f"Unsafe Speaker overlay: original={n_old}, patched={n_new}")
    updated = src.replace(before, after, 1)
    if updated.replace(after, before, 1) != src:
        raise RuntimeError("Unexpected modification outside the exact R6 Speaker block")
    return updated, style


def main() -> int:
    head = subprocess.check_output(
        ["git", "-C", str(UPSTREAM), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != PINNED:
        raise RuntimeError(f"Unexpected M5Unified revision: {head}")
    before, after = blocks(R6_OVERLAY.read_text(encoding="utf-8"))
    original = SOURCE.read_bytes()
    patched, style = apply_source(original, before, after)
    if patched != original:
        SOURCE.write_bytes(patched)
        if SOURCE.read_bytes() != patched:
            raise RuntimeError("Speaker patch write-back verification failed")
    print(f"PASS TAB5PLATFORM_R6_SPEAKER_EOL: {style}, exact R6 patch verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
