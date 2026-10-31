#!/usr/bin/env python3
"""Tab5Platform ESP-IDF 5.5.4 Windows native build, no flash, one result ZIP.

Runs the real build, not a probe. Does not touch flash, partition data, or other checkouts.
"""
from __future__ import annotations

import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
RESULT_DIR = ROOT / "native_build_result"
REPORT_ZIP = ROOT / "Tab5Platform_NATIVE_BUILD_RESULT.zip"
BUILD_LOG = RESULT_DIR / "BUILD.log"
RESULT_JSON = RESULT_DIR / "RESULT.json"
APP_BIN = ROOT / "build" / "tab5_platform_reference.bin"
IDF_FALLBACK = Path(r"G:\esp-idf-5.5.4")


def run_build(command: str) -> int:
    print("Tab5Platform: ESP-IDF 5.5.4 native build (no flash)", flush=True)
    process = subprocess.Popen(
        ["cmd.exe", "/d", "/s", "/c", command],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        bufsize=1,
    )
    with BUILD_LOG.open("w", encoding="utf-8") as log:
        for line in process.stdout:
            print(line, end="", flush=True)
            log.write(line)
    return process.wait()


def current_commit() -> str | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True, timeout=10,
        )
        return result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return None


def main() -> int:
    RESULT_DIR.mkdir(exist_ok=True)
    started = dt.datetime.now(dt.timezone.utc)
    rc = 1
    source = "none"
    detail = ""
    if os.name != "nt":
        detail = "This build launcher requires Windows cmd.exe and installed ESP-IDF 5.5.4"
        BUILD_LOG.write_text(detail + "\n", encoding="utf-8")
    else:
        installed = os.environ.get("IDF_PATH", "").strip()
        idf_root = Path(installed) if installed else IDF_FALLBACK
        export = idf_root / "export.bat"
        if not export.is_file():
            detail = f"ESP-IDF export.bat not found at {export}"
            BUILD_LOG.write_text(detail + "\n", encoding="utf-8")
        else:
            source = str(idf_root)
            # The checked-out source must be built in its own folder. No writes to X68KTab5.
            # The bootstrap script verifies both pinned M5 repositories and applies guarded overlays.
            command = (
                f'call "{export}" && '
                'python tools\\bootstrap_deps.py && '
                'idf.py set-target esp32p4 && '
                'idf.py build'
            )
            try:
                rc = run_build(command)
            except OSError as exc:
                detail = f"Unable to launch build: {exc}"
                BUILD_LOG.write_text(detail + "\n", encoding="utf-8")

    result = {
        "format": "TAB5PLATFORM-NATIVE-BUILD-RESULT-v1",
        "started_at_utc": started.isoformat(),
        "completed_at_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "git_commit": current_commit(),
        "idf_root": source,
        "target": "esp32p4",
        "command": "bootstrap pinned libraries; idf.py set-target esp32p4; idf.py build",
        "flash_performed": False,
        "exit_code": rc,
        "build_pass": rc == 0,
        "detail": detail,
        "app_bin": None,
    }
    # Never label an old BIN as an output when the build failed.
    if rc == 0 and APP_BIN.is_file():
        digest = hashlib.sha256()
        with APP_BIN.open("rb") as source_bin:
            while chunk := source_bin.read(1024 * 1024):
                digest.update(chunk)
        result["app_bin"] = {
            "filename": APP_BIN.name,
            "bytes": APP_BIN.stat().st_size,
            "sha256": digest.hexdigest(),
        }
    if rc == 0 and not result["app_bin"]:
        result["build_pass"] = False
        result["exit_code"] = 6
        result["detail"] = "idf.py returned success but the expected app BIN was not found"
    RESULT_JSON.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with zipfile.ZipFile(REPORT_ZIP, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.write(BUILD_LOG, "BUILD.log")
        archive.write(RESULT_JSON, "RESULT.json")
        if result["app_bin"]:
            archive.write(APP_BIN, APP_BIN.name)
    print(f"ONE REPORT: {REPORT_ZIP}", flush=True)
    print(f"RESULT: {'PASS' if result['build_pass'] else 'FAIL'}; no flash performed", flush=True)
    return 0 if result["build_pass"] else (rc if rc != 0 else 6)


if __name__ == "__main__":
    sys.exit(main())
