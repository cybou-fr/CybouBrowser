# Packaging (R5, first step)

Build a runnable, self-contained folder and a ZIP of it:

```bash
cmake --build build --config Release --target cybou
cmake --install build --config Release --prefix build/stage
(cd build && cpack -C Release -G ZIP)   # build/CYBOU-<version>-win64.zip
```

The install step runs `windeployqt` (Qt DLLs, QML modules, WebEngine process and resources, translations), copies the
Visual C++ runtime next to the executable, and installs `licenses/` and the README.

## Installer (Qt Installer Framework 4.11)

```bash
bash tools/package/build_installer.sh   # build/CYBOU-<version>-setup.exe, about 158 MB, from build/stage
```

Per-user install to `%LOCALAPPDATA%\Programs\CYBOU` (no administrator), Start menu and desktop shortcuts, uninstaller
`CYBOUUninstall.exe` (also listed in the Windows uninstall list). Sources: `installer/`.

## SBOM

```bash
python -I tools/package/make_sbom.py build/stage build/CYBOU-sbom.cdx.json
```

CycloneDX 1.5 JSON: every shipped file with SHA-256, grouped into components (CYBOU, Qt 6.12.0, Qt WebEngine/Chromium 140.0.7339.225,
MSVC runtime, DirectX compiler, Mesa software OpenGL, docs). Deterministic: the same staged folder gives a byte-identical file
(checked). Component versions of Qt and Chromium are the build's, the others are not read from the binaries and have no version.
Not covered: Chromium's own third-party libraries inside `Qt6WebEngineCore.dll` (the engine is one component here), package
URLs/CPE identifiers, and signing of the SBOM.

## Reproducible build

`bash tools/package/repro_check.sh` builds `cybou.exe` twice from clean and compares SHA-256. Needs `/Brepro` (set in
`CMakeLists.txt` for MSVC) and `SOURCE_DATE_EPOCH` (the script sets it): without the latter `rcc` embeds the modification time of a
resource file and the two builds differ in 3 bytes. Result: two clean builds in the same source folder with the same toolchain give a
byte-identical `cybou.exe` (`4e279ccb...`). Not shown: a different folder (absolute paths may be embedded), a different machine,
the Qt DLLs (taken from the Qt install, not built here) and the installer (IFW stamps).

## Signing

`powershell -ExecutionPolicy Bypass -File tools/package/sign_selfsigned.ps1 <files>` signs with a self-signed certificate
(`CN=CYBOU Development (self-signed)`, created in `CurrentUser\My`, never added to a trusted root). Windows reports the status as
`UnknownError` (untrusted publisher) and SmartScreen still warns; a modified file reports `HashMismatch`. It exercises the signing step
and makes files tamper-evident by thumbprint. A trusted signature needs a certificate from a CA, a signing service or the Store.

## One-command release candidate

`bash tools/package/release.sh` -> `build/release/`: builds, stages, signs `cybou.exe` (self-signed), writes the SBOM from the signed
stage, makes the ZIP and the installer, signs the installer, writes `SHA256SUMS`. Checked once: the ZIP contains the signed
`cybou.exe` (same thumbprint) and starts with a reduced `PATH`; the installer is signed. Qt DLLs and `QtWebEngineProcess.exe` are not
signed by CYBOU. The installer's inner payload is the same signed stage. Not checked: installing from this installer after the change
(the install/uninstall check above was done on the earlier, unsigned build).

## Updates: none, by decision (2026-10-28)

CYBOU has **no automatic or online update**. Installers are built `--offline-only` and carry no repository address, so neither the browser nor its maintenance tool fetches anything; `build_repo.sh` and `CYBOU_UPDATE_URL` were removed. A new release is installed by hand by the user.

Why: the negative tests of the former Qt Installer Framework update channel (`docs/VALIDATION.md`, 2026-10-28) showed that an update killed mid-way left the installation without its files and could not repair itself, and that a substituted repository with self-consistent checksums was accepted. Until an update mechanism can keep the previous version until the new one is complete and authenticate the publisher, none is shipped.

**Installing a new release over the old one.** The user runs the new setup. Qt Installer Framework refuses a folder that already holds an installation (`TargetDirectoryInUse`), so each release installs into its own folder, `%LOCALAPPDATA%\Programs\CYBOU-<version>` (`TargetDir` uses `@ProductVersion@`; `ifw_prepare.sh` sets the installer version from the package version). `installer/packages/org.cybou.browser/meta/installscript.qs` then runs, in order: extract the new files (a damaged archive fails here), remove the previous release with its own maintenance tool (`purge`; found through `HKCU\Software\CYBOU\InstallDir`, written by the last setup, or the old fixed folder `Programs\CYBOU`), create the shortcuts for the new release, record its folder. The browser profile under `%APPDATA%` is outside every install folder. Silent installs must not pass `--root`, which would replace the per-release folder with a fixed one.

`python tools/package/install_over_test.py` checks this on a stand-in product with the same install script (2026-10-28): a new release installs over the old one and the old one is gone; a tampered installer leaves the installed release intact (it hangs instead of failing and was stopped after 5 minutes); an installation killed mid-way leaves the previous release usable and completes when the setup is run again. Not yet run with the real CYBOU installers over a real installation: that run removes the installed release.

## Checked

- Installer, silent (`setup.exe --root <dir> --accept-licenses --accept-messages --confirm-command install`): exit 0, 271 files,
  `bin/cybou.exe` and the uninstaller present, Start menu shortcut points at the installed executable. Uninstall
  (`CYBOUUninstall.exe purge --confirm-command`): exit 0, the install folder, Start menu entry and desktop shortcut are gone.
  The interactive wizard pages and the "launch after install" option were not exercised.

- The staged folder and the ZIP extracted to a fresh directory start `bin/cybou.exe` with `PATH` reduced to
  `C:\Windows\System32;C:\Windows` (no Qt in `PATH`): Qt libraries are loaded from the package, the window opens and
  `QtWebEngineProcess.exe` starts. Alpha 1 ZIP is about 160 MB (250 files).

## Not done (R5 stays open)

- Machine-wide install (the installer is per-user), upgrade-over-existing-install behaviour, user data handling on uninstall
  (the browser profile under `%APPDATA%` is not touched).
- A trusted code-signing certificate, authenticated updates over HTTPS, automatic rollback.
- A complete third-party license bundle (`licenses/THIRD_PARTY_NOTICES.md` is a short notice only) and an SBOM that goes inside Chromium.
- Run on a machine that never had Qt, Visual Studio or the VC redistributable: only simulated here by `PATH`.
