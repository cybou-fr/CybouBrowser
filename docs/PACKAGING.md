# Packaging (R5, first step)

Build a runnable, self-contained folder and a ZIP of it:

```bash
cmake --build build --config Release --target cybou
cmake --install build --config Release --prefix build/stage
(cd build && cpack -C Release -G ZIP)   # build/CYBOU-<version>-win64.zip
```

The install step runs `windeployqt` (Qt DLLs, QML modules, WebEngine process and resources, translations), copies the
Visual C++ runtime next to the executable, and installs `licenses/` and the README.

## Checked

- The staged folder and the ZIP extracted to a fresh directory start `bin/cybou.exe` with `PATH` reduced to
  `C:\Windows\System32;C:\Windows` (no Qt in `PATH`): Qt libraries are loaded from the package, the window opens and
  `QtWebEngineProcess.exe` starts. Alpha 1 ZIP is about 160 MB (250 files).

## Not done (R5 stays open)

- A real installer (no NSIS/WiX installed here; Qt Installer Framework is available under `C:\Qt\Tools`), Start menu entry,
  uninstall, per-user vs machine install.
- Code signing (needs a certificate), update channel with rollback.
- SBOM and a complete third-party license bundle (`licenses/THIRD_PARTY_NOTICES.md` is a short notice only).
- Reproducible build (same inputs, same hash) is not demonstrated.
- Run on a machine that never had Qt, Visual Studio or the VC redistributable: only simulated here by `PATH`.
