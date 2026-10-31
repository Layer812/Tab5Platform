#!/usr/bin/env python3
"""Dependency-free LF/CRLF/idempotency regression for the R6 Speaker adapter."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from apply_r6_speaker_overlay import apply_source, blocks

before = b"          else\n" + b"          line\n" * 10 + b"          }\n"
after = b"          else\n" + b"X68KTAB_SPK_FAST_WAKE_R1\n" + b"          new_line\n" * 10 + b"          }\n"
assert blocks(f"BEFORE = {before!r}\nAFTER = {after!r}\n") == (before, after)
prefix = b"if (flg_nodata)\ni2s_config.clk_cfg.sample_rate_hz = _cfg.sample_rate;\n"

for style in ("LF", "CRLF"):
    eol = b"\r\n" if style == "CRLF" else b"\n"
    old = (prefix + before).replace(b"\n", eol)
    expected = (prefix + after).replace(b"\n", eol)
    modified, mode = apply_source(old, before, after)
    assert modified == expected and mode == style
    repeated, mode = apply_source(modified, before, after)
    assert repeated == modified and mode == style + " (already applied)"
    try:
        apply_source(old + before.replace(b"\n", eol), before, after)
    except RuntimeError as error:
        assert "Unsafe Speaker overlay" in str(error)
    else:
        raise AssertionError(f"duplicate block accepted for {style}")
print("PASS pinned-R6 Speaker adapter LF/CRLF, idempotency, duplicate rejection")
