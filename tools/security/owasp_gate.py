#!/usr/bin/env python3
"""CYBOU security baseline and OWASP evidence gate. Python 3.10+, standard library only.
No external scans and no user-profile access. Release fails closed until official
ASVS/WSTG inventories are pinned, every control is triaged, and evidence exists.
"""
import argparse
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
SUITES=("security","controlbar","transport","tracker","surface","smoke","pna","webrtc","hsts","download","protocol","url-parser","fuzz","geolocation","wasm","media","sandbox","media-nodevice")
ASVS_ID=re.compile(r"(?:v5\.0\.0-|V)?(\d{1,2}\.\d{1,2}\.\d{1,2})")
WSTG_ID=re.compile(r"WSTG-(?:v42-)?([A-Z]{4}-\d{2})")
ISO_DATE=re.compile(r"\d{4}-\d{2}-\d{2}")

def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))

def sha(path):
    dig=hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""):
            dig.update(chunk)
    return dig.hexdigest()

def revision():
    try:
        r=subprocess.run(["git","rev-parse","HEAD"],cwd=ROOT,
                         text=True,capture_output=True,timeout=10,check=True)
        return r.stdout.strip()
    except (OSError,subprocess.SubprocessError):
        return "unknown"

def inventory(asvs, wstg, lock):
    errors=[]
    if not asvs or not wstg:
        return set(),["Official ASVS CSV and WSTG ID index must be supplied offline"]
    standards=read_json(lock)
    inputs=[("asvs",asvs),("wstg",wstg)]
    for key,path in inputs:
        if not Path(path).is_file():
            errors.append(f"{key}: input missing: {path}")
            continue
        pin=standards.get(key,{})
        if not pin.get("sha256"):
            errors.append(f"{key}: SHA-256 not pinned in standards.lock.json")
        elif sha(path).lower()!=pin["sha256"].lower():
            errors.append(f"{key}: SHA-256 mismatch; source changed")
    if errors:
        return set(),errors
    result=set()
    with Path(asvs).open(encoding="utf-8-sig",newline="") as f:
        reader=csv.DictReader(f)
        if not reader.fieldnames or "req_id" not in reader.fieldnames:
            return set(),["ASVS CSV requires the official req_id column"]
        for row in reader:
            cell=(row.get("req_id") or "").strip()
            match=ASVS_ID.fullmatch(cell)
            if not match:
                errors.append("Malformed ASVS requirement ID: "+cell)
                continue
            key="ASVS:v5.0.0-"+match.group(1)
            if key in result: errors.append("Duplicate ASVS requirement ID: "+cell)
            result.add(key)
    with Path(wstg).open(encoding="utf-8-sig") as f:
        for line in f:
            cell=line.strip()
            if not cell: continue
            match=WSTG_ID.fullmatch(cell)
            if not match:
                errors.append("Malformed WSTG ID: "+cell)
                continue
            key="WSTG:v4.2-"+match.group(1)
            if key in result: errors.append("Duplicate WSTG ID: "+cell)
            result.add(key)
    for key,prefix,minimum in [("asvs","ASVS:",100),("wstg","WSTG:",50)]:
        count=sum(x.startswith(prefix) for x in result)
        expected=standards.get(key,{}).get("requirementCount")
        if count<minimum or (expected is not None and count!=expected):
            errors.append(f"{key}: requirement count mismatch ({count}, pinned {expected})")
    source=standards.get("wstg",{})
    if source.get("sourcePath"):
        source_path=ROOT/source["sourcePath"]
        if not source_path.is_file() or sha(source_path)!=source.get("sourceSha256"):
            errors.append("WSTG source checklist SHA-256 mismatch or missing")
        else:
            ids=re.findall(r"^\|\s*(WSTG-[A-Z]{4}-\d{2})\s*\|",source_path.read_text(encoding="utf-8-sig"),re.M)
            derived={"WSTG:v4.2-"+WSTG_ID.fullmatch(x).group(1) for x in ids}
            if len(ids)!=len(set(ids)) or derived!={x for x in result if x.startswith("WSTG:")}:
                errors.append("WSTG index differs from pinned source checklist")
    return result,errors

def case_index(path):
    data=read_json(path)
    cases=data.get("cases",[])
    ids=[x.get("id") for x in cases]
    errors=[]
    if len(ids)!=len(set(ids)) or any(not i for i in ids):
        errors.append("Duplicate or missing case id")
    for x in cases:
        if not x.get("domain") or not x.get("method") or not x.get("tier"):
            errors.append("Incomplete case metadata: "+str(x.get("id")))
    return {x['id']:x for x in cases if x.get('id')},errors,len(ids)

def current_pass_evidence(item, commit, binary_sha):
    if not item or not commit or commit=="unknown" or not binary_sha:
        return False
    if item.get("result")!="PASS" or item.get("commit")!=commit or item.get("binarySha256")!=binary_sha:
        return False
    if not item.get("log") or not item.get("logSha256"):
        return False
    try:
        path=Path(item["log"])
        if not path.is_absolute(): path=ROOT/path
        return path.is_file() and path.stat().st_size>0 and item["logSha256"]==sha(path)
    except OSError:
        return False

def check_coverage(requirements, coverage_file, evidence_file, cases, commit, binary_sha, reviewers=frozenset()):
    errors=[]
    rows=read_json(coverage_file).get("requirements",[])
    mapping={}
    for row in rows:
        key=row.get("id")
        if key in mapping:
            errors.append("Duplicate requirement entry: "+str(key))
        mapping[key]=row
    absent=requirements-set(mapping)
    extra=set(mapping)-requirements
    if absent: errors.append(f"{len(absent)} official controls untriaged")
    if extra: errors.append(f"{len(extra)} mapped controls missing from pinned standards")
    evidences=read_json(evidence_file).get("evidence",[]) if Path(evidence_file).exists() else []
    evidence={}
    for item in evidences:
        case_id=item.get("caseId")
        if not case_id or case_id in evidence:
            errors.append("Duplicate or missing evidence caseId: "+str(case_id))
        if case_id not in cases:
            errors.append("Evidence references unknown case: "+str(case_id))
        evidence[case_id]=item

    # Browser-specific risks are independently mandatory for release, even when not in ASVS.
    if commit=="unknown":
        errors.append("Current git revision cannot be independently established")
    for case_id,item in cases.items():
        if item.get("status")!="verified":
            errors.append(case_id+": browser-specific case not verified")
        e=evidence.get(case_id)
        if not current_pass_evidence(e,commit,binary_sha):
            errors.append(case_id+": no current-sha/hash PASS evidence")
    applicable=0
    skipped=0
    for key in sorted(requirements & set(mapping)):
        row=mapping[key]
        disposition=row.get("disposition")
        if disposition=="not_applicable":
            skipped+=1
            if not row.get("rationale") or not row.get("reviewedBy"):
                errors.append(key+": N/A requires reason and reviewer")
            # A reviewer is only accepted when listed by name in reviewers.json, with a dated, referenced review.
            elif row["reviewedBy"] not in reviewers:
                errors.append(key+": N/A reviewer is not listed in reviewers.json")
            if not ISO_DATE.fullmatch(str(row.get("reviewedOn",""))) or not row.get("reviewRef"):
                errors.append(key+": N/A requires reviewedOn (YYYY-MM-DD) and reviewRef")
            # The chapter-level proposal text is shared by many rows; an approval needs an item-specific reason.
            if row.get("rationale") and row.get("rationale")==row.get("proposalRationale"):
                errors.append(key+": N/A rationale repeats the chapter-level proposal")
        elif disposition=="applicable":
            applicable+=1
            mapped=row.get("caseIds") or []
            if not mapped or not row.get("owner"):
                errors.append(key+": missing owner or caseIds")
            # PASS logs prove a case ran, not that it covers this requirement: require the named assertions.
            if row.get("verificationStatus")!="verified" or not row.get("assertions"):
                errors.append(key+": verificationStatus not verified or no mapped assertions")
            for cid in mapped:
                if cid not in cases:
                    errors.append(key+": unknown case "+str(cid))
                    continue
                e=evidence.get(cid)
                if not current_pass_evidence(e,commit,binary_sha):
                    errors.append(key+": missing current-revision PASS evidence for "+cid)
        else:
            errors.append(key+": disposition must be applicable/not_applicable")
    if not requirements:
        errors.append("No pinned official requirements loaded")
    return errors,{"total":len(requirements),"mapped":len(requirements & set(mapping)),
                   "applicable":applicable,"notApplicable":skipped,
                   "untriaged":len(absent)+sum(mapping[k].get("disposition") not in ("applicable","not_applicable") for k in requirements & set(mapping)),"extra":len(extra)}

ENGINE_LINE=re.compile(r'ENGINE "?([\d.]+)"? "?([\d.]+)"? "?([\d.]+)"? "?security patch"? "?([\d.]+)"?')

ENGINE_CONFIG=re.compile(r"ENGINE_CONFIG strictWebRtc=(\d) disableWasm=(\d) pna=(\d) unsafe=(\S+)")
# Suites that measure one engine switch in child processes of their own; every other suite stands for the product.
CONFIG_SPECIFIC_SUITES=("webrtc","wasm","sandbox")

def engine_config_errors(suites):
    """Every product suite must report the shipped protections (strict WebRTC, WebAssembly off, private network
    preflights) and no setting that disables engine protection; otherwise its PASS does not describe the product."""
    errors=[]; seen={}
    for s in suites:
        if s.get("suite") in CONFIG_SPECIFIC_SUITES or s.get("status")=="BLOCKED": continue
        try:
            m=ENGINE_CONFIG.search(Path(s.get("log","")).read_text(encoding="utf-8",errors="replace"))
        except OSError:
            m=None
        if not m:
            errors.append(f"{s.get('suite')}: no ENGINE_CONFIG line: engine protections of this run unknown"); continue
        strict,wasm,pna,unsafe=m.groups()
        seen[s["suite"]]=m.group(0)
        off=[name for name,value in (("strictWebRtc",strict),("disableWasm",wasm),("pna",pna)) if value!="1"]
        if off: errors.append(f"{s['suite']}: ran without product protections: {', '.join(off)}")
        if unsafe!="none": errors.append(f"{s['suite']}: ran with unsafe engine settings: {unsafe}")
    return errors,seen

def engine_runtime_errors(suites, sbom_file):
    """cybou_tests prints the runtime Qt API versions (qVersion, qWebEngineVersion, qWebEngineChromiumVersion,
    qWebEngineChromiumSecurityPatchVersion); they must equal what the SBOM read from the staged binaries."""
    if not Path(sbom_file).is_file():
        return ["SBOM missing: runtime engine versions cannot be compared"],None
    for s in suites:
        try:
            m=ENGINE_LINE.search(Path(s.get("log","")).read_text(encoding="utf-8",errors="replace"))
        except OSError:
            continue
        if m:
            break
    else:
        return ["no ENGINE line in suite logs: runtime engine versions unknown"],None
    v=sbom_versions(sbom_file)
    runtime=dict(zip(("qt","webengine","chromium","patch"),m.groups()))
    shipped={"qt":v.get(("qt",None)),"webengine":v.get(("qt-webengine-chromium",None)),
             "chromium":v.get(("qt-webengine-chromium","cybou:chromium-version")),"patch":v.get(PATCH_LEVEL["chromium"])}
    return [f"runtime {k} {runtime[k]} differs from SBOM {shipped[k]}" for k in runtime if runtime[k]!=shipped[k]],runtime

def run_suites(executable, output_dir, timeout):
    results=[]
    if not executable or not Path(executable).is_file():
        return [{"suite":s,"status":"BLOCKED","reason":"cybou_tests executable missing"} for s in SUITES]
    output_dir.mkdir(parents=True,exist_ok=True)
    for suite in SUITES:
        log=output_dir/(suite+".log")
        try:
            with log.open("w",encoding="utf-8") as f:
                proc=subprocess.run([str(Path(executable).resolve()),"--"+suite+"-test"],
                                    cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=timeout)
            results.append({"suite":suite,"status":"PASS" if proc.returncode==0 and log.stat().st_size > 0 else "FAIL",
                            "exitCode":proc.returncode,"log":str(log),"sha256":sha(log),
                            "logBytes":log.stat().st_size})
        except subprocess.TimeoutExpired:
            results.append({"suite":suite,"status":"FAIL","reason":"timeout","log":str(log)})
        except OSError as ex:
            results.append({"suite":suite,"status":"BLOCKED","reason":str(ex)})
    return results

# execution: executable check of browser behaviour; framework: catalog/mapping completeness; process: assurance review.
CASE_KINDS=("execution","framework","process")
THREAT_ID=re.compile(r"(?:CWE|CAPEC)-\d+")
VEX_STATUS=("not_affected","affected","fixed","under_investigation")
# How a fixed/not_affected decision was reached, and how close that evidence is to the shipped bytes.
# binary: checked against the shipped engine (regression test); source: the shipped Qt sources inspected;
# branch: a backport commit exists before the build, presence in the binary not shown; declared: vendor claim only.
EVIDENCE_LEVEL={"binary-test":"binary","source-inspection":"source","backport-commit":"branch"}
RELEASE_LEVELS=("binary","source")

def canonical_sha(obj):
    """SHA-256 of a JSON value in canonical form; feeds record it so later edits are detected."""
    return hashlib.sha256(json.dumps(obj,sort_keys=True,ensure_ascii=False,separators=(",",":")).encode("utf-8")).hexdigest()
# Vulnerability-feed package names -> (SBOM bom-ref, SBOM property holding the version, or None for "version").
COMPONENTS={"chromium":("qt-webengine-chromium","cybou:chromium-version"),
            "qt":("qt",None),"qtwebengine":("qt-webengine-chromium",None)}
# Components whose vendor cherry-picks upstream security fixes up to a declared level (SBOM property).
PATCH_LEVEL={"chromium":("qt-webengine-chromium","cybou:chromium-security-patch-version")}

def patch_level_covers(record, version, level):
    """Every affecting range is fixed upstream at or below the vendor's security patch level."""
    fixes=[r.get("fixed") for r in record.get("ranges",[]) if affects({"ranges":[r]},version)]
    try:
        return bool(fixes) and all(f and version_key(f)<=version_key(level) for f in fixes)
    except TypeError:
        return False

def check_framework(requirements, coverage_file, cases, threats_file):
    """Completeness of the framework itself: every control decided or proposed, links resolve, threats mapped.
    No executable or evidence is needed; that is the job of the test and release gates."""
    errors=[]
    rows={r.get("id"):r for r in read_json(coverage_file).get("requirements",[])}
    if set(rows)!=requirements:
        errors.append(f"coverage differs from pinned standards ({len(requirements-set(rows))} missing, {len(set(rows)-requirements)} extra)")
    for key,row in sorted(rows.items()):
        d=row.get("disposition")
        if d=="applicable":
            if not row.get("owner") or not row.get("caseIds"): errors.append(key+": applicable row needs owner and caseIds")
            errors+=[f"{key}: unknown case {c}" for c in row.get("caseIds",[]) if c not in cases]
        elif d=="not_applicable":
            if not row.get("rationale"): errors.append(key+": N/A row needs a rationale")
        elif not row.get("proposedDisposition") or not (row.get("proposalRationale") or row.get("proposalNote")):
            errors.append(key+": undecided row needs a recorded proposal and reason")
    threats=read_json(threats_file).get("threats",[]) if Path(threats_file).exists() else []
    if not threats: errors.append("threat catalog missing or empty")
    seen=set()
    for x in threats:
        tid=str(x.get("id"))
        if not THREAT_ID.fullmatch(tid) or tid in seen: errors.append("malformed or duplicate threat id: "+tid)
        seen.add(tid)
        if not x.get("title") or not x.get("caseIds"): errors.append(tid+": threat needs title and caseIds")
        errors+=[f"{tid}: unknown case {c}" for c in x.get("caseIds",[]) if c not in cases]
    for cid,case in sorted(cases.items()):
        if case.get("kind") not in CASE_KINDS: errors.append(f"{cid}: kind must be one of {', '.join(CASE_KINDS)}")
        sources=case.get("testSources",[])
        if case.get("status") in ("in_progress","verified") and not sources:
            errors.append(cid+": "+case["status"]+" case lists no testSources")
        errors+=[f"{cid}: test source missing: {s}" for s in sources if not (ROOT/s).is_file()]
    threat_cases={c for x in threats for c in x.get("caseIds",[])}
    linked={c for r in rows.values() for c in r.get("caseIds",[])}|threat_cases
    # Gaps are reported, not failed: the catalog is expected to grow.
    return errors,{"threats":len(threats),"cases":len(cases),
                   "kinds":{k:sum(v.get("kind")==k for v in cases.values()) for k in CASE_KINDS},
                   "executionP0":sorted(c for c,v in cases.items() if v.get("kind")=="execution" and v.get("tier")=="P0"),
                   "p0WithoutThreat":sorted(c for c,v in cases.items() if v.get("tier")=="P0" and c not in threat_cases),
                   "casesWithoutLink":sorted(set(cases)-linked)}

def version_key(v):
    return tuple(int(x) if x.isdigit() else x for x in re.split(r"[.\-]",str(v)))

def affects(record, version):
    if version in record.get("versions",[]): return True
    for r in record.get("ranges",[]):
        lo,hi,last=r.get("introduced","0"),r.get("fixed"),r.get("lastAffected")
        try:
            v=version_key(version)
            if v>=version_key(lo) and (hi is None or v<version_key(hi)) and (last is None or v<=version_key(last)): return True
        except TypeError:
            continue
    return False

def snapshot_age_error(name, data, today, max_age):
    try:
        age=(today-dt.date.fromisoformat(str(data.get("retrievedOn")))).days
    except ValueError:
        return name+": retrievedOn missing; import a current snapshot"
    return name+f": snapshot is {age} days old (max {max_age})" if age>max_age else None

def sbom_versions(sbom_file):
    """(bom-ref, None) -> component version and (bom-ref, property name) -> property value."""
    versions={}
    for c in read_json(sbom_file).get("components",[]):
        versions[(c.get("bom-ref"),None)]=c.get("version")
        for prop in c.get("properties",[]): versions[(c.get("bom-ref"),prop.get("name"))]=prop.get("value")
    return versions

def qt_source_identity(qt_src):
    """(Qt WebEngine version, Chromium security patch level) of a Qt WebEngine source tree, or None."""
    try:
        conf=(Path(qt_src)/".cmake.conf").read_text(encoding="utf-8")
        chromium=(Path(qt_src)/"CHROMIUM_VERSION").read_text(encoding="utf-8")
    except OSError:
        return None
    version=re.search(r'QT_REPO_MODULE_VERSION "([^"]+)"',conf)
    level=re.search(r"security patches up to Chromium version:\s*(\S+)",chromium)
    return (version.group(1) if version else None, level.group(1) if level else None)

def source_binding_errors(rid, decision, versions, qt_src):
    """A source-inspection decision names the Qt WebEngine tree and the hashed files it read. The tree must be the
    release the SBOM ships (version and patch level) and, where the tree is present, the files must hash the same.
    Returns (errors, verified)."""
    b=decision.get("sourceBinding") or {}
    shipped=(versions.get(("qt-webengine-chromium",None)),versions.get(PATCH_LEVEL["chromium"]))
    if not b.get("files"):
        return [f"{rid}: source-inspection needs sourceBinding with hashed files"],False
    if (b.get("webengine"),b.get("patchLevel"))!=shipped:
        return [f"{rid}: inspected Qt WebEngine {b.get('webengine')}/{b.get('patchLevel')} is not the shipped {shipped[0]}/{shipped[1]}"],False
    if qt_source_identity(qt_src)!=shipped:
        return [],False
    root=Path(qt_src)/"src/3rdparty/chromium"
    bad=[f["path"] for f in b["files"] if not (root/f["path"]).is_file() or sha(root/f["path"])!=f.get("sha256")]
    return ([f"{rid}: inspected file differs from the Qt source tree: {', '.join(bad)}"] if bad else []),not bad

def check_risk(sbom_file, vulns_dir, today, max_age=7, strict=False, qt_src=None):
    """Known vulnerabilities (normalized CVE/OSV snapshot + CISA KEV) against the shipped SBOM, with VEX-style triage.
    strict (release): every resolution must rest on binary or source evidence, not on a branch commit or a vendor claim."""
    errors=[]; vulns_dir=Path(vulns_dir)
    if not Path(sbom_file).is_file():
        return ["SBOM missing: run tools/package/make_sbom.py on the staged package"],{}
    versions=sbom_versions(sbom_file)
    snap,kev,vex=(read_json(vulns_dir/n) if (vulns_dir/n).exists() else {} for n in ("snapshot.json","kev.json","triage.json"))
    for name,data in (("snapshot.json",snap),("kev.json",kev)):
        problem=snapshot_age_error(name,data,today,max_age)
        if problem: errors.append(problem)
    for name,data,key in (("snapshot.json",snap,"records"),("kev.json",kev,"cveIds")):
        if data and data.get("contentSha256")!=canonical_sha(data.get(key,[])):
            errors.append(name+": content does not match contentSha256 (edited after import or not imported)")
    kev_ids=set(kev.get("cveIds",[]))
    triage={(d.get("vulnId"),d.get("component")):d for d in vex.get("decisions",[])}
    bp=read_json(vulns_dir/"qt-backports.json") if (vulns_dir/"qt-backports.json").exists() else {}
    if bp and bp.get("contentSha256")!=canonical_sha(bp.get("cves",{})):
        errors.append("qt-backports.json: content does not match contentSha256")
    backports=bp.get("cves",{})
    findings=[]
    for rec in snap.get("records",[]):
        rid,comp=rec.get("id"),rec.get("component")
        if comp not in COMPONENTS:
            errors.append(f"{rid}: unknown component {comp}"); continue
        version=versions.get(COMPONENTS[comp])
        if not version:
            errors.append(f"{rid}: SBOM has no version for {comp}"); continue
        if not affects(rec,version): continue
        in_kev=bool({rid,*rec.get("aliases",[])}&kev_ids)
        d=triage.get((rid,comp),{})
        status=d.get("status")
        level=versions.get(PATCH_LEVEL.get(comp))
        # Automatic only with both facts: the vendor's declared level covers the fix and a backport commit exists.
        # An explicit triage decision always wins; KEV entries are reported but resolved the same way.
        if status is None and level and rid in backports and patch_level_covers(rec,version,level):
            status="patch-level"
        evidence=("branch+declared" if status=="patch-level" else
                  EVIDENCE_LEVEL.get(d.get("method")) if status in ("fixed","not_affected","affected") else None)
        verified=None
        if d.get("method")=="source-inspection":
            problems,verified=source_binding_errors(rid,d,versions,qt_src)
            errors+=problems
            if strict and not verified and not problems:
                errors.append(f"{rid}: source binding not verified here (Qt WebEngine sources of the shipped release not found)")
        findings.append({"id":rid,"component":comp,"version":version,"kev":in_kev,"status":status,"evidence":evidence,
                         "sourceVerified":verified})
        resolved=status in ("fixed","not_affected","patch-level")
        if status in ("fixed","not_affected") and (d.get("method") not in EVIDENCE_LEVEL or not d.get("evidence")):
            errors.append(f"{rid}: {status} needs a method ({', '.join(EVIDENCE_LEVEL)}) and evidence links")
        elif status=="not_affected" and not d.get("justification"):
            errors.append(f"{rid}: not_affected needs a justification")
        elif resolved and strict and evidence not in RELEASE_LEVELS:
            errors.append(f"{rid}: {status} rests on {evidence} evidence; release needs binary or source evidence")
        elif resolved:
            pass
        elif status not in VEX_STATUS or status=="under_investigation":
            errors.append(f"{rid}: {comp} {version} matches and is not triaged")
        elif status=="affected":
            errors.append(f"{rid}: known vulnerability affects shipped {comp} {version}"+(" (CISA KEV)" if in_kev else ""))
    return errors,{"records":len(snap.get("records",[])),"matches":len(findings),
                   "kev":sum(f["kev"] for f in findings),
                   "patchLevel":sum(f["status"]=="patch-level" for f in findings),
                   "byEvidence":{lvl:sum(f["evidence"]==lvl for f in findings) for lvl in ("binary","source","branch","branch+declared")},
                   "triaged":sum(f["status"] in VEX_STATUS and f["status"]!="under_investigation" for f in findings),
                   "open":sum(f["status"] not in VEX_STATUS+("patch-level",) or f["status"]=="under_investigation" for f in findings),
                   "findings":findings}

def suite_markers(suites):
    """suite name -> text of its log in this run, only for suites that passed."""
    logs={}
    for s in suites:
        if s.get("status")!="PASS": continue
        try: logs[s["suite"]]=Path(s["log"]).read_text(encoding="utf-8",errors="replace")
        except OSError: pass
    return logs

def run_tool(script, timeout=3600):
    try:
        r=subprocess.run([sys.executable,str(ROOT/script)],cwd=ROOT,capture_output=True,text=True,timeout=timeout)
        return r.stdout+r.stderr if r.returncode==0 else ""
    except (OSError,subprocess.SubprocessError):
        return ""

def check_r3(cases, suites, vulns_dir, tool_runner=run_tool):
    """0.5.0 acceptance (R3): every P0 execution case is proven by its evidence markers in this run, has no open
    required scenario (a scenario prefixed [rc] is deferred to the release candidate), and every CISA KEV match is
    fixed, not affected, or affected with a mitigation in place by default. Framework and process cases do not gate."""
    errors=[]; table={}
    logs=suite_markers(suites)
    tools={}
    for cid,case in sorted(cases.items()):
        if case.get("kind")!="execution" or case.get("tier")!="P0": continue
        evidence=case.get("evidence",[])
        missing=[]
        for e in evidence:
            source=e.get("suite","")
            if source.startswith("tool:"):
                if source not in tools: tools[source]=tool_runner(source[5:])
                text=tools[source]
            else:
                text=logs.get(source,"")
            if e.get("marker","") not in text: missing.append(f"{source}:{e.get('marker')}")
        open_=[s for s in case.get("remainingScenarios",[]) if not s.startswith("[rc]")]
        table[cid]={"status":case.get("status"),"evidence":len(evidence),"missing":missing,"open":len(open_)}
        if not evidence: errors.append(f"{cid}: no executable evidence")
        if missing: errors.append(f"{cid}: evidence not produced in this run: {', '.join(missing)}")
        if open_: errors.append(f"{cid}: {len(open_)} required scenario(s) open")
        if case.get("status") not in ("in_progress","verified"): errors.append(f"{cid}: status {case.get('status')}")
    vulns_dir=Path(vulns_dir)
    kev=set(read_json(vulns_dir/"kev.json").get("cveIds",[])) if (vulns_dir/"kev.json").exists() else set()
    decisions={d.get("vulnId"):d for d in (read_json(vulns_dir/"triage.json").get("decisions",[]) if (vulns_dir/"triage.json").exists() else [])}
    records=read_json(vulns_dir/"snapshot.json").get("records",[]) if (vulns_dir/"snapshot.json").exists() else []
    if not kev or not records: errors.append("KEV or vulnerability snapshot missing: known exploited vulnerabilities cannot be checked")
    for rid in sorted({r.get("id") for r in records if r.get("id") in kev or set(r.get("aliases",[]))&kev}):
        d=decisions.get(rid,{})
        ok=(d.get("status") in ("fixed","not_affected") and d.get("evidence")) or \
           (d.get("status")=="affected" and (d.get("mitigation") or {}).get("status")=="in_place_by_default")
        if not ok: errors.append(f"{rid}: CISA KEV match neither fixed, not affected nor mitigated by default")
    return errors,{"cases":table}

def reviewer_names(path):
    if not Path(path).exists(): return frozenset()
    return frozenset(x["name"] for x in read_json(path).get("reviewers",[]) if x.get("name"))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    # framework: the tooling is complete; test: suites run; risk: known vulnerabilities; release: all of them, strictly.
    # baseline and inventory are the earlier names of test and of the coverage/evidence check.
    # r3: the 0.5.0 acceptance gate (test suites + P0 execution evidence + known exploited vulnerabilities).
    p.add_argument("--mode",choices=("framework","test","risk","r3","release","baseline","inventory"),default="test")
    p.add_argument("--exe",help="Path to cybou_tests executable")
    p.add_argument("--asvs-csv",default=str(ROOT/"tests/security/standards/asvs-5.0.0.csv"),help="Official OWASP ASVS 5.0.0 CSV (offline)")
    p.add_argument("--wstg-ids",default=str(ROOT/"tests/security/standards/wstg-4.2-ids.txt"),help="Pinned canonical WSTG v4.2 IDs, one per line")
    p.add_argument("--timeout",type=int,default=300)
    p.add_argument("--lock",default=str(ROOT/"tests/security/standards.lock.json"))
    p.add_argument("--cases",default=str(ROOT/"tests/security/cases.json"))
    p.add_argument("--coverage",default=str(ROOT/"tests/security/coverage.json"))
    p.add_argument("--reviewers",default=str(ROOT/"tests/security/reviewers.json"))
    p.add_argument("--evidence",default=str(ROOT/"tests/security/evidence.json"))
    p.add_argument("--threats",default=str(ROOT/"tests/security/threats.json"))
    p.add_argument("--sbom",default=str(ROOT/"build/CYBOU-sbom.cdx.json"))
    p.add_argument("--vulns",default=str(ROOT/"tests/security/vulns"))
    p.add_argument("--max-snapshot-age",type=int,default=7)
    p.add_argument("--qt-src",help="Qt WebEngine source tree of the shipped release (default: C:/Qt/<qt>/Src/qtwebengine)")
    p.add_argument("--out",default=str(ROOT/"build/security-report.json"))
    a=p.parse_args()
    if a.mode=="baseline": a.mode="test"
    out=Path(a.out)
    out.parent.mkdir(parents=True,exist_ok=True)
    commit=revision()
    case_ids,case_errors,case_count=case_index(a.cases)
    report={"schemaVersion":1,"timestamp":dt.datetime.now(dt.timezone.utc).isoformat(),
            "commit":commit,"platform":platform.platform(),"python":sys.version.split()[0],
            "mode":a.mode,"caseCount":case_count,"suites":[],"coverage":{},
            "errors":list(case_errors)}
    binarysha=sha(a.exe) if a.exe and Path(a.exe).is_file() else ""
    report["binarySha256"]=binarysha
    try:
        diff=subprocess.run(["git","diff","HEAD","--binary"],cwd=ROOT,
                            capture_output=True,timeout=10,check=True).stdout
        status=subprocess.run(["git","status","--porcelain"],cwd=ROOT,
                              capture_output=True,timeout=10,check=True).stdout
        report["workingTreeDirty"]=bool(status.strip())
        report["sourceDiffSha256"]=hashlib.sha256(diff).hexdigest()
    except (OSError,subprocess.SubprocessError):
        report["workingTreeDirty"]=None
    if a.mode=="release" and report["workingTreeDirty"] is not False:
        report["errors"].append("Release requires a verified clean working tree")
    if a.mode in ("test","r3","release"):
        report["suites"]=run_suites(a.exe,out.parent/"security-logs",a.timeout)
        if any(x["status"]!="PASS" for x in report["suites"]):
            report["errors"].append("Baseline suites not all PASS")
        problems,report["engineRuntime"]=engine_runtime_errors(report["suites"],a.sbom)
        report["errors"]+=problems
        problems,report["engineConfig"]=engine_config_errors(report["suites"])
        report["errors"]+=problems
    if a.mode in ("inventory","release"):
        requirements,problems=inventory(a.asvs_csv,a.wstg_ids,a.lock)
        report["errors"]+=problems
        problems,stats=check_coverage(requirements,a.coverage,a.evidence,case_ids,commit,binarysha,reviewer_names(a.reviewers))
        report["errors"]+=problems
        report["coverage"]=stats
        if a.mode=="release" and not binarysha:
            report["errors"].append("Release candidate binary hash missing")
    if a.mode in ("framework","release"):
        requirements,problems=inventory(a.asvs_csv,a.wstg_ids,a.lock)
        report["errors"]+=problems
        problems,report["framework"]=check_framework(requirements,a.coverage,case_ids,a.threats)
        report["errors"]+=problems
        # N/A grounds: architecture facts must hold in the product sources and docs/NA_REPORT.md must be current.
        import na_report
        text,problems=na_report.build()
        if not na_report.REPORT.is_file() or na_report.REPORT.read_text(encoding="utf-8")!=text:
            problems.append("docs/NA_REPORT.md is stale: run tools/security/na_report.py")
        report["errors"]+=problems
    if a.mode=="r3":
        problems,report["r3"]=check_r3(case_ids,report["suites"],a.vulns)
        report["errors"]+=problems
    if a.mode in ("risk","release"):
        qt_src=a.qt_src
        if not qt_src and Path(a.sbom).is_file():
            qt_src=f"C:/Qt/{sbom_versions(a.sbom).get(('qt',None))}/Src/qtwebengine"
        problems,report["risk"]=check_risk(a.sbom,a.vulns,dt.date.today(),a.max_snapshot_age,
                                           strict=a.mode=="release",qt_src=qt_src)
        report["errors"]+=problems
    out.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print("CYBOU security gate:", "PASS" if not report["errors"] else "BLOCKED",
          "|",len(report["errors"]),"issues | report:",out)
    for error in report["errors"][:30]: print(" -",error,file=sys.stderr)
    if len(report["errors"])>30: print("...more in JSON report",file=sys.stderr)
    return 0 if not report["errors"] else 2

if __name__=="__main__":
    sys.exit(main())
