"""CycloneDX 1.5 SBOM of a staged CYBOU package: every shipped file with its SHA-256, grouped into components.

    python tools/package/make_sbom.py build/stage build/CYBOU-sbom.cdx.json

Output is deterministic (sorted, no timestamp, serial number derived from the file hashes), so the same
staged folder always gives the same SBOM. Versions of Qt and Chromium are taken from the build, not guessed from names.
"""
import hashlib, json, re, sys
from pathlib import Path

# Qt, Qt WebEngine and Chromium versions are read from the staged binaries (see binary_versions), not hard-coded.
PRODUCT_VERSION = "0.4.0-alpha.1"

def resource_version(path, key="FileVersion"):
    """Version string from a PE VS_VERSIONINFO block (UTF-16), e.g. 6.12.0.0 -> 6.12.0."""
    data, k = path.read_bytes(), key.encode("utf-16-le")
    for m in re.finditer(re.escape(k), data):
        value = data[m.end():m.end() + 64].decode("utf-16-le", "ignore").lstrip("\x00").split("\x00")[0]
        if re.fullmatch(r"\d+(\.\d+){1,3}", value):
            return ".".join(value.split(".")[:3])
    sys.exit("no %s in %s" % (key, path))

def binary_versions(stage):
    """Qt from Qt6Core.dll, Qt WebEngine from its own resource, and the two Chromium strings Qt WebEngine embeds:
    the base version (qWebEngineChromiumVersion) and the security patch level (qWebEngineChromiumSecurityPatchVersion),
    which is never lower than the base."""
    core, engine = stage / "bin" / "Qt6Core.dll", stage / "bin" / "Qt6WebEngineCore.dll"
    found = sorted({m.decode() for m in re.findall(rb"(?<![\d.])1\d{2}\.0\.\d{4}\.\d{1,3}(?![\d.])", engine.read_bytes())},
                   key=lambda v: tuple(map(int, v.split("."))))
    if len(found) not in (1, 2):
        sys.exit("expected the Chromium base and patch versions in %s, found %s" % (engine, found))
    return {"qt": resource_version(core), "webengine": resource_version(engine),
            "chromium": found[0], "chromium_patch": found[-1]}

def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def classify(rel, v):
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
        return "qt", "library", v["qt"]
    if re.match(r"(vcruntime|msvcp|concrt|vccorlib)", name):
        return "msvc-runtime", "library", None
    if name.startswith("qt6webengine") or name == "qtwebengineprocess.exe" or "webengine" in rel.parts or name.endswith(".pak") or name in ("icudtl.dat", "v8_context_snapshot.bin"):
        return "qt-webengine-chromium", "library", v["webengine"]
    if name.startswith("qt6") or "qml" in rel.parts or "plugins" in rel.parts or "translations" in rel.parts:
        return "qt", "library", v["qt"]
    if rel.parts[0] in ("licenses",) or name == "readme.md":
        return "cybou-docs", "file", PRODUCT_VERSION
    return "other", "file", None

def main(stage, out):
    stage = Path(stage)
    files = sorted(p for p in stage.rglob("*") if p.is_file())
    v = binary_versions(stage)
    groups = {}
    for p in files:
        rel = p.relative_to(stage)
        key, kind, version = classify(rel, v)
        g = groups.setdefault(key, {"kind": kind, "version": version, "files": []})
        g["files"].append({"path": rel.as_posix(), "sha256": sha256(p), "size": p.stat().st_size})
    meta = {
        "cybou": ("CYBOU Browser", "proprietary (pre-release)", "The browser application; CYBOU's own Qt Positioning plugin is compiled into it"),
        "qt": ("Qt", "LGPL-3.0-only", "Qt %s libraries, QML modules and plugins, unmodified, dynamically linked" % v["qt"]),
        "qt-webengine-chromium": ("Qt WebEngine %s (Chromium %s)" % (v["webengine"], v["chromium"]), "LGPL-3.0-only; Chromium and bundled libraries under their own licenses (BSD-3-Clause and others)", "Browser engine, process, resources and locales"),
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
        if key == "qt-webengine-chromium":
            comp["properties"].append({"name": "cybou:chromium-version", "value": v["chromium"]})
            comp["properties"].append({"name": "cybou:chromium-security-patch-version", "value": v["chromium_patch"]})
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
