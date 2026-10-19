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
- SBOM and a complete third-party license bundle (`licenses/THIRD_PARTY_NOTICES.md` is a short notice only).
- Reproducible build (same inputs, same hash) is not demonstrated.
- Run on a machine that never had Qt, Visual Studio or the VC redistributable: only simulated here by `PATH`.
