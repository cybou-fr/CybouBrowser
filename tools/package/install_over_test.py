#!/usr/bin/env python3
"""CYB-UPD-003: a new release installed by hand over the previous one (installer/packages/org.cybou.browser/meta/installscript.qs).

  python tools/package/install_over_test.py [--ifw C:/Qt/Tools/QtInstallerFramework/4.11/bin]

Builds offline installers of a small stand-in product with CYBOU's install script (version marker + filler file), one
folder per release as in the product, installs only under build/install-over and purges at the end; the real CYBOU
installation is not touched. A file outside the install folders stands for the browser profile. Prints INSTALL_OVER_*
lines; exit 0 only if every protection holds.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "build" / "install-over"
PROGRAMS = WORK / "Programs"
PKG = "org.cybou.installovertest"
NAME = "CYBOU-InstallOverTest"
TOOL = "InstallOverMaintenance"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def folder(version):
    return PROGRAMS / f"{NAME}-{version}.0.0"


def build_setup(ifw, version, filler_mb):
    """Offline installer for one stand-in release; returns (setup.exe, sha256 of its filler)."""
    base = WORK / f"v{version}"
    pkg = base / "packages" / PKG
    write(pkg / "meta" / "package.xml", f'<?xml version="1.0"?>\n<Package><DisplayName>Install over test</DisplayName>'
          f'<Description>stand-in</Description><Version>{version}.0.0</Version><ReleaseDate>2026-10-28</ReleaseDate>'
          f'<Default>true</Default><ForcedInstallation>true</ForcedInstallation><Script>installscript.qs</Script></Package>\n')
    shutil.copy(ROOT / "installer" / "packages" / "org.cybou.browser" / "meta" / "installscript.qs", pkg / "meta" / "installscript.qs")
    write(pkg / "data" / "version.txt", f"v{version}")
    filler = pkg / "data" / "filler.bin"
    with filler.open("wb") as f:
        for _ in range(filler_mb):
            f.write(os.urandom(1024 * 1024))  # incompressible, so extraction takes long enough to be interrupted
    write(base / "config.xml", f'<?xml version="1.0"?>\n<Installer><Name>{NAME}</Name><Version>{version}.0.0</Version>'
          f'<Title>Install over test</Title><Publisher>CYBOU tests</Publisher>'
          f'<TargetDir>{PROGRAMS.as_posix()}/{NAME}-@ProductVersion@</TargetDir>'
          f'<MaintenanceToolName>{TOOL}</MaintenanceToolName><RemoveTargetDir>true</RemoveTargetDir></Installer>\n')
    setup = WORK / f"setup{version}.exe"
    subprocess.run([str(ifw / "binarycreator.exe"), "--offline-only", "-c", str(base / "config.xml"), "-p", str(base / "packages"), str(setup)],
                   check=True, capture_output=True)
    return setup, sha(filler)


def installed():
    """Version markers of every release folder present."""
    if not PROGRAMS.exists():
        return []
    return sorted(f.read_text(encoding="utf-8") for f in PROGRAMS.glob(f"{NAME}-*/version.txt"))


def complete(version, filler_sha):
    d = folder(version)
    return (d / "version.txt").exists() and (d / "version.txt").read_text(encoding="utf-8") == f"v{version}" \
        and (d / "filler.bin").exists() and sha(d / "filler.bin") == filler_sha and (d / f"{TOOL}.exe").exists()


def setup_run(setup, timeout=900):
    # No --root: it would replace the per-release folder with one fixed folder.
    try:
        r = subprocess.run([str(setup), "--accept-licenses", "--accept-messages", "--confirm-command", "install"],
                           capture_output=True, text=True, timeout=timeout)
        return r.returncode, (r.stdout + r.stderr)[-600:]
    except subprocess.TimeoutExpired:
        subprocess.run(["taskkill", "/F", "/T", "/IM", setup.name], capture_output=True)
        return "timeout", ""


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    found = sorted(Path("C:/Qt/Tools/QtInstallerFramework").glob("*/bin"))
    p.add_argument("--ifw", default=found[-1] if found else None)
    a = p.parse_args()
    if not a.ifw:
        print("INSTALL_OVER BLOCKED: Qt Installer Framework not found"); return 2
    ifw = Path(a.ifw)
    shutil.rmtree(WORK, ignore_errors=True)
    profile = WORK / "profile" / "settings.ini"   # stands for %APPDATA%\CYBOU, outside every install folder
    write(profile, "kept")
    results = []

    def check(name, ok, detail=""):
        results.append((name, ok))
        print(f"INSTALL_OVER_CASE {'PASS' if ok else 'FAIL'} {name} {detail}".rstrip(), flush=True)

    s1, f1 = build_setup(ifw, 1, 8)
    s2, f2 = build_setup(ifw, 2, 8)
    s3, f3 = build_setup(ifw, 3, 300)
    try:
        code, out = setup_run(s1)
        if code != 0 or not complete(1, f1):
            print("INSTALL_OVER BLOCKED: first install failed", code, out); return 2

        # 1. A new release over the old one: the new one lands whole, then the old one is removed by its own tool.
        code, out = setup_run(s2)
        check("new-release-over-old", code == 0 and complete(2, f2) and installed() == ["v2"] and profile.read_text() == "kept",
              f"exit {code}, installed {installed()}, profile kept {profile.read_text() == 'kept'}")

        # 2. A tampered installer must not leave the user without the installed release.
        tampered = WORK / "setup3-tampered.exe"
        data = bytearray(s3.read_bytes()); data[len(data) * 3 // 4] ^= 0xFF; tampered.write_bytes(bytes(data))
        code, out = setup_run(tampered, 300)
        intact = complete(2, f2) and installed() == ["v2"]
        check("tampered-installer-keeps-old", code != 0 and intact, f"exit {code}, installed {installed()}, old release intact {intact}")
        if not intact:
            setup_run(s2)  # restore for the next case

        # 3. An installation killed mid-way keeps the old release usable and completes when the setup is run again.
        proc = subprocess.Popen([str(s3), "--accept-licenses", "--accept-messages", "--confirm-command", "install"],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(4)
        killed = proc.poll() is None
        if killed:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
            proc.wait(30)
        after_kill = installed()
        still_usable = complete(2, f2)
        code, out = setup_run(s3)
        check("interrupted-install-keeps-old-then-completes", killed and still_usable and code == 0 and complete(3, f3)
              and installed() == ["v3"] and profile.read_text() == "kept",
              f"killed {killed}, after kill {after_kill}, v2 still usable {still_usable}, rerun exit {code}, installed {installed()}")
        if code != 0:
            print("INSTALL_OVER_RERUN_OUTPUT", out.replace("\n", " "))
    finally:
        for tool in sorted(PROGRAMS.glob(f"{NAME}-*/{TOOL}.exe")) if PROGRAMS.exists() else []:
            subprocess.run([str(tool), "purge", "--confirm-command"], capture_output=True, timeout=300)
        time.sleep(2)
        left = [p.name for p in PROGRAMS.iterdir()] if PROGRAMS.exists() else []
        print("INSTALL_OVER_CLEANUP", "all releases removed" if not left else f"left: {left}")

    failed = [n for n, ok in results if not ok]
    if failed:
        print("INSTALL_OVER FAIL:", ", ".join(failed)); return 1
    print("INSTALL_OVER PASS: a new release installs over the old one (old removed by its own tool after the new files are on disk, "
          "profile outside kept); a tampered installer leaves the old release intact; an interrupted installation keeps the old "
          "release usable and completes when the setup is run again")
    return 0


if __name__ == "__main__":
    sys.exit(main())
