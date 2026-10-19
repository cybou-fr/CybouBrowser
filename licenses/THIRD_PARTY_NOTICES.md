# Third-party components shipped with CYBOU

CYBOU is built on the following components. This file is a short notice, not a complete bill of materials
(a file-level CycloneDX SBOM with SHA-256 hashes is produced by `tools/package/make_sbom.py`, see docs/PACKAGING.md).

| Component | Version | License | How it is used |
|---|---|---|---|
| Qt (Core, Gui, Quick, QuickControls2, Network, Positioning, WebEngine) | 6.12.0 | LGPL-3.0-only (also available under GPL-3.0 and a commercial license) | Dynamically linked Qt libraries (`bin/Qt6*.dll`), unmodified |
| Qt WebEngine (Chromium 140.0.7339.225 and its third-party code) | 6.12.0 | LGPL-3.0-only for Qt WebEngine; Chromium and its bundled libraries under their own licenses (BSD-3-Clause and others) | `Qt6WebEngineCore.dll`, `QtWebEngineProcess.exe`, `resources/` |
| Microsoft D3DCompiler_47, DirectX Shader Compiler, Mesa software OpenGL (`opengl32sw.dll`) | as shipped with Qt 6.12.0 | Microsoft redistributable terms; Apache-2.0 with LLVM exception; MIT (Mesa) | Direct3D shader compilation and software GL fallback, redistributed unmodified |
| Microsoft Visual C++ runtime | per installed redistributable | Microsoft redistributable terms | Required at run time |

## Qt under LGPL-3.0

The Qt libraries are used unmodified and linked dynamically. You may replace them with another build of the same
Qt version. The LGPL-3.0 and GPL-3.0 texts are in this directory. Qt source code: https://code.qt.io/ and
https://download.qt.io/official_releases/qt/6.12/. Chromium source: https://source.chromium.org/chromium.

## Not shipped

The stock Qt WinRT positioning plugin is replaced at run time by CYBOU's own plugin (compiled into the executable).
A patched copy of the Qt plugin exists under `tools/qt-winrt-positioning-fix/` for research only and is not shipped.

Full license texts for Chromium's third-party code are produced by Qt WebEngine's build (`chrome://credits`
equivalent); including them verbatim in the package is open work.
