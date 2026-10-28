#!/usr/bin/env python3
"""CYB-UPD-003: negative tests of the update channel CYBOU ships (Qt Installer Framework repository + maintenance tool).

  python tools/package/update_integrity_test.py [--ifw C:/Qt/Tools/QtInstallerFramework/4.11/bin]

Uses a small stand-in product on the same mechanism (version marker + filler file) so that every scenario takes
seconds instead of a 400 MB browser repository. Everything lives in build/upd-test and is purged at the end; the
real CYBOU installation is not touched. Prints UPDATE_INTEGRITY_* lines; exit 0 only if every protection holds.
The repository substitution scenario documents a known gap (no publisher authentication) and is reported as such.
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
WORK = ROOT / "build" / "upd-test"
PKG = "org.cybou.updatetest"
FILLER_MB = 64  # large enough for the interrupted update to be caught mid-way


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def package_tree(base, version, marker, filler_seed, random_mb=0):
    """IFW package directory: meta/package.xml and data/{version.txt, filler.bin}."""
    pkg = base / "packages" / PKG
    write(pkg / "meta" / "package.xml", f'<?xml version="1.0"?>\n<Package><DisplayName>Update test</DisplayName>'
          f'<Description>stand-in</Description><Version>{version}</Version><ReleaseDate>2026-10-28</ReleaseDate>'
          f'<Default>true</Default><ForcedInstallation>true</ForcedInstallation></Package>\n')
    write(pkg / "data" / "version.txt", marker)
    filler = pkg / "data" / "filler.bin"
    block = hashlib.sha256(filler_seed.encode()).digest() * (1024 * 32)  # 1 MiB, deterministic per seed
    with filler.open("wb") as f:
        for _ in range(FILLER_MB):
            f.write(block)
        # Incompressible tail so that the interrupted-update case has a long enough extraction to be caught.
        for _ in range(random_mb):
            f.write(os.urandom(1024 * 1024))
    return base / "packages"


def repogen(ifw, packages, out):
    shutil.rmtree(out, ignore_errors=True)
    subprocess.run([str(ifw / "repogen.exe"), "-p", str(packages), str(out)], check=True, capture_output=True)


def run(cmd, timeout=600):
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return r.returncode, (r.stdout + r.stderr)[-800:]


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--ifw", default=sorted(Path("C:/Qt/Tools/QtInstallerFramework").glob("*/bin"))[-1] if Path("C:/Qt/Tools/QtInstallerFramework").exists() else None)
    a = p.parse_args()
    if not a.ifw:
        print("UPDATE_INTEGRITY BLOCKED: Qt Installer Framework not found"); return 2
    ifw = Path(a.ifw)
    shutil.rmtree(WORK, ignore_errors=True)
    repo = WORK / "repo"
    install = WORK / "install"
    tool = install / "UpdTestMaintenance.exe"
    results, gaps = [], []

    def check(name, ok, detail=""):
        results.append((name, ok, detail))
        print(f"UPDATE_INTEGRITY_CASE {'PASS' if ok else 'FAIL'} {name} {detail}".rstrip(), flush=True)

    def installed():
        f = install / "version.txt"
        return f.read_text(encoding="utf-8") if f.exists() else None

    # v1 installer with the repository URL baked in, as tools/package/ifw_prepare.sh does for CYBOU_UPDATE_URL.
    v1 = WORK / "v1"
    packages = package_tree(v1, "1.0.0", "v1", "v1")
    write(v1 / "config" / "config.xml", f'<?xml version="1.0"?>\n<Installer><Name>CYBOU-UpdateTest</Name><Version>1.0.0</Version>'
          f'<Title>Update test</Title><Publisher>CYBOU tests</Publisher><TargetDir>{install.as_posix()}</TargetDir>'
          f'<MaintenanceToolName>UpdTestMaintenance</MaintenanceToolName><RemoveTargetDir>true</RemoveTargetDir>'
          f'<RemoteRepositories><Repository><Url>{repo.as_uri()}</Url></Repository></RemoteRepositories></Installer>\n')
    setup = WORK / "setup.exe"
    # No --offline-only: it drops the repository address (as tools/package/build_installer.sh notes).
    subprocess.run([str(ifw / "binarycreator.exe"), "-c", str(v1 / "config" / "config.xml"), "-p", str(packages), str(setup)],
                   check=True, capture_output=True)
    repogen(ifw, packages, repo)
    code, out = run([str(setup), "--root", str(install), "--accept-licenses", "--accept-messages", "--confirm-command", "install"])
    if code != 0 or installed() != "v1":
        print("UPDATE_INTEGRITY BLOCKED: installing v1 failed", code, out); return 2

    def publish(version, marker, seed, random_mb=0):
        packages = package_tree(WORK / f"pkg-{version}", version, marker, seed, random_mb)
        repogen(ifw, packages, repo)
        return sha(packages / PKG / "data" / "filler.bin")

    def update(timeout=600):
        return run([str(tool), "up", "--accept-licenses", "--accept-messages", "--confirm-command"], timeout)

    try:
        # 1. Control: a genuine new version is applied.
        publish("2.0.0", "v2", "v2")
        code, out = update()
        check("control-update", code == 0 and installed() == "v2", f"exit {code}, installed {installed()}")
        before = sha(install / "filler.bin")

        # 2. Corrupted package: one byte of the published archive flipped, metadata untouched.
        publish("3.0.0", "v3", "v3")
        archive = next(repo.rglob("*.7z"))
        data = bytearray(archive.read_bytes()); data[len(data) // 2] ^= 0xFF; archive.write_bytes(bytes(data))
        code, out = update()
        check("corrupted-archive-refused", code != 0 and installed() == "v2" and sha(install / "filler.bin") == before,
              f"exit {code}, installed {installed()}")

        # 3. Unreachable source: the repository is gone.
        shutil.rmtree(repo)
        code, out = update(120)
        check("unreachable-source-refused", code != 0 and installed() == "v2" and sha(install / "filler.bin") == before,
              f"exit {code}, installed {installed()}")

        # 4. Substituted repository: whoever can write the repository publishes their own build with valid metadata.
        publish("5.0.0", "attacker", "attacker")
        code, out = update()
        accepted = code == 0 and installed() == "attacker"
        print(f"UPDATE_INTEGRITY_GAP repository-substitution {'ACCEPTED' if accepted else 'refused'}: the framework verifies only the "
              f"checksums the repository itself publishes; there is no publisher signature", flush=True)
        if accepted: gaps.append("repository-substitution")
        # 5. Interrupted update (last: it may leave the installation broken): kill the maintenance tool mid-way, then run it again.
        v6_filler = publish("6.0.0", "v6", "v6", random_mb=300)
        proc = subprocess.Popen([str(tool), "up", "--accept-licenses", "--accept-messages", "--confirm-command"],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.8)
        killed = proc.poll() is None
        if killed:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
            proc.wait(30)
        midway = installed()
        left = sorted(p.name for p in install.iterdir()) if install.exists() else []
        recorded = (install / "components.xml").read_text(encoding="utf-8", errors="replace") if (install / "components.xml").exists() else ""
        print("UPDATE_INTEGRITY_INTERRUPTED left", left, "recorded versions", [l.strip() for l in recorded.splitlines() if "<Version>" in l][:3], flush=True)
        code_check, _ = run([str(tool), "ch", "--confirm-command"], 120)
        code, out = update()
        # Success is the final state, whatever the rerun's exit code (it may have nothing left to do).
        complete = installed() == "v6" and (install / "filler.bin").exists() and sha(install / "filler.bin") == v6_filler
        check("interrupted-update-recovers", killed and tool.exists() and code_check == 0 and complete,
              f"killed {killed}, marker after kill {midway}, check exit {code_check}, rerun exit {code}, installed {installed()}, filler intact {complete}")

    finally:
        purge = run([str(tool), "purge", "--confirm-command"], 300) if tool.exists() else (None, "no maintenance tool")
        time.sleep(2)  # the maintenance tool removes itself and its folder after exiting
        print("UPDATE_INTEGRITY_CLEANUP", f"purge exit {purge[0]},", "install removed" if not install.exists() else "install folder remains")

    failed = [n for n, ok, _ in results if not ok]
    if failed:
        print("UPDATE_INTEGRITY FAIL:", ", ".join(failed)); return 1
    print("UPDATE_INTEGRITY PASS: genuine update applied; corrupted archive and unreachable source refused with the installation "
          "unchanged; an interrupted update leaves a working maintenance tool and completes on rerun"
          + (f"; KNOWN GAP: {', '.join(gaps)}" if gaps else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
