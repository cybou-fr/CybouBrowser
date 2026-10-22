"""CycloneDX 1.5 SBOM of a staged CYBOU package: every shipped file with its SHA-256, grouped into components.

    python tools/package/make_sbom.py build/stage build/CYBOU-sbom.cdx.json

Output is deterministic (sorted, no timestamp, serial number derived from the file hashes), so the same
staged folder always gives the same SBOM. Versions of Qt and Chromium are taken from the build, not guessed from names.
"""
import hashlib, json, re, sys
from pathlib import Path

QT_VERSION = "6.12.0"
CHROMIUM_VERSION = "140.0.7339.225"
PRODUCT_VERSION = "0.3.0-alpha.2"

def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def classify(rel):
    name = rel.name.lower()
    if name == "cybou.exe":
        return "cybou", "application", PRODUCT_VERSION
    if name == "d3dcompiler_47.dll":
        return "directx-compiler", "library", None
    if name == "dxcompiler.dll":
        return "directx-shader-compiler", "library", None
    if name == "opengl32sw.dll":
        return "mesa-software-opengl", "library", None
    if name == "qt.conf":
        return "qt", "library", QT_VERSION
    if re.match(r"(vcruntime|msvcp|concrt|vccorlib)", name):
        return "msvc-runtime", "library", None
    if name.startswith("qt6webengine") or name == "qtwebengineprocess.exe" or "webengine" in rel.parts or name.endswith(".pak") or name in ("icudtl.dat", "v8_context_snapshot.bin"):
        return "qt-webengine-chromium", "library", QT_VERSION
    if name.startswith("qt6") or "qml" in rel.parts or "plugins" in rel.parts or "translations" in rel.parts:
        return "qt", "library", QT_VERSION
    if rel.parts[0] in ("licenses",) or name == "readme.md":
        return "cybou-docs", "file", PRODUCT_VERSION
    return "other", "file", None

def main(stage, out):
    stage = Path(stage)
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    groups = {}
    for p in files:
        rel = p.relative_to(stage)
        key, kind, version = classify(rel)
        g = groups.setdefault(key, {"kind": kind, "version": version, "files": []})
        g["files"].append({"path": rel.as_posix(), "sha256": sha256(p), "size": p.stat().st_size})
    meta = {
        "cybou": ("CYBOU Browser", "proprietary (pre-release)", "The browser application; CYBOU's own Qt Positioning plugin is compiled into it"),
        "qt": ("Qt", "LGPL-3.0-only", "Qt 6.12.0 libraries, QML modules and plugins, unmodified, dynamically linked"),
        "qt-webengine-chromium": ("Qt WebEngine (Chromium %s)" % CHROMIUM_VERSION, "LGPL-3.0-only; Chromium and bundled libraries under their own licenses (BSD-3-Clause and others)", "Browser engine, process, resources and locales"),
        "directx-compiler": ("Microsoft D3DCompiler_47", "Microsoft redistributable terms", "HLSL compiler redistributed with Qt's Direct3D support"),
        "directx-shader-compiler": ("DirectX Shader Compiler (dxcompiler.dll)", "LLVM-Exception Apache-2.0 / University of Illinois NCSA", "Shader compiler redistributed with Qt; version not read from the binary"),
        "mesa-software-opengl": ("Mesa software OpenGL (opengl32sw.dll, llvmpipe)", "MIT (Mesa) with LLVM Apache-2.0 with LLVM exception", "Software OpenGL fallback redistributed with Qt; version not read from the binary"),
        "msvc-runtime": ("Microsoft Visual C++ runtime", "Microsoft redistributable terms", "Redistributed from the build machine's Visual Studio"),
        "cybou-docs": ("CYBOU documentation and licenses", "see files", "README and license texts"),
        "other": ("Other files", "unknown", "Files not matched to a component: review before release"),
    }
    components = []
    for key in sorted(groups):
        g = groups[key]
        name, lic, desc = meta[key]
        comp = {"type": g["kind"], "bom-ref": key, "name": name, "description": desc,
                "licenses": [{"license": {"name": lic}}],
                "properties": [{"name": "cybou:file-count", "value": str(len(g["files"]))}],
                "components": [{"type": "file", "bom-ref": key + ":" + f["path"], "name": f["path"],
                                "hashes": [{"alg": "SHA-256", "content": f["sha256"]}]} for f in g["files"]]}
        if g["version"]:
            comp["version"] = g["version"]
        components.append(comp)
    digest = hashlib.sha256("".join(f["sha256"] for g in groups.values() for f in g["files"]).encode()).hexdigest()
    bom = {"bomFormat": "CycloneDX", "specVersion": "1.5",
           "serialNumber": "urn:uuid:%s-%s-%s-%s-%s" % (digest[:8], digest[8:12], digest[12:16], digest[16:20], digest[20:32]),
           "version": 1,
           "metadata": {"component": {"type": "application", "name": "CYBOU", "version": PRODUCT_VERSION, "bom-ref": "cybou-package"},
                        "tools": {"components": [{"type": "application", "name": "tools/package/make_sbom.py"}]}},
           "components": components}
    Path(out).write_text(json.dumps(bom, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print("SBOM:", out, "-", len(files), "files in", len(components), "components; other:", len(groups.get("other", {}).get("files", [])))

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
