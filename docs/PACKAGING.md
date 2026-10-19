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
- Code signing (needs a certificate), update channel with rollback.
- A complete third-party license bundle (`licenses/THIRD_PARTY_NOTICES.md` is a short notice only) and an SBOM that goes inside Chromium.
- Reproducible build (same inputs, same hash) is not demonstrated.
- Run on a machine that never had Qt, Visual Studio or the VC redistributable: only simulated here by `PATH`.
