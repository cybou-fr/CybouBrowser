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
SUITES=("security","controlbar","transport","tracker","surface","smoke")
ASVS_ID=re.compile(r"(?:v5\.0\.0-)?(\d{1,2}\.\d{1,2}\.\d{1,2})")
WSTG_ID=re.compile(r"WSTG-(?:v42-)?([A-Z]{4}-\d{2})")

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
        for row in csv.reader(f):
            # Look for a complete requirement ID in a CSV cell, never from a prose snippet.
            for cell in row:
                c=cell.strip()
                m=ASVS_ID.fullmatch(c)
                if m:
                    result.add("ASVS:v5.0.0-"+m.group(1))
                    break
    if len([x for x in result if x.startswith("ASVS:")])<100:
        errors.append("ASVS inventory suspiciously small (<100 requirements)")
    with Path(wstg).open(encoding="utf-8-sig") as f:
        for line in f:
            c=line.strip()
            m=WSTG_ID.fullmatch(c)
            if m:
                result.add("WSTG:v4.2-"+m.group(1))
    if len([x for x in result if x.startswith("WSTG:")])<50:
        errors.append("WSTG inventory suspiciously small (<50 scenarios)")
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

def check_coverage(requirements, coverage_file, evidence_file, cases, commit, binary_sha):
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
    evidence={x.get("caseId"):x for x in evidences}

    # Browser-specific risks are independently mandatory for release, even when not in ASVS.
    if commit=="unknown":
        errors.append("Current git revision cannot be independently established")
    for case_id,item in cases.items():
        if item.get("status")!="verified":
            errors.append(case_id+": browser-specific case not verified")
        e=evidence.get(case_id)
        if not e or e.get("result")!="PASS" or e.get("commit")!=commit \
           or e.get("binarySha256")!=binary_sha or not e.get("log") \
           or not Path(e["log"]).is_file() \
           or e.get("logSha256")!=sha(e["log"]):
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
        elif disposition=="applicable":
            applicable+=1
            mapped=row.get("caseIds") or []
            if not mapped or not row.get("owner"):
                errors.append(key+": missing owner or caseIds")
            for cid in mapped:
                if cid not in cases:
                    errors.append(key+": unknown case "+str(cid))
                    continue
                e=evidence.get(cid)
                if not e or e.get("result")!="PASS" or e.get("commit")!=commit \
                   or e.get("binarySha256")!=binary_sha \
                   or not e.get("log") or not Path(e["log"]).is_file():
                    errors.append(key+": missing current-revision PASS evidence for "+cid)
        else:
            errors.append(key+": disposition must be applicable/not_applicable")
    if not requirements:
        errors.append("No pinned official requirements loaded")
    return errors,{"total":len(requirements),"mapped":len(requirements & set(mapping)),
                   "applicable":applicable,"notApplicable":skipped,
                   "untriaged":len(absent),"extra":len(extra)}

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
            results.append({"suite":suite,"status":"PASS" if proc.returncode==0 else "FAIL",
                            "exitCode":proc.returncode,"log":str(log),"sha256":sha(log)})
        except subprocess.TimeoutExpired:
            results.append({"suite":suite,"status":"FAIL","reason":"timeout","log":str(log)})
        except OSError as ex:
            results.append({"suite":suite,"status":"BLOCKED","reason":str(ex)})
    return results

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--mode",choices=("baseline","inventory","release"),default="baseline")
    p.add_argument("--exe",help="Path to cybou_tests executable")
    p.add_argument("--asvs-csv",help="Official OWASP ASVS 5.0.0 CSV (offline)")
    p.add_argument("--wstg-ids",help="Pinned canonical WSTG v4.2 IDs, one per line")
    p.add_argument("--timeout",type=int,default=300)
    p.add_argument("--lock",default=str(ROOT/"tests/security/standards.lock.json"))
    p.add_argument("--cases",default=str(ROOT/"tests/security/cases.json"))
    p.add_argument("--coverage",default=str(ROOT/"tests/security/coverage.json"))
    p.add_argument("--evidence",default=str(ROOT/"tests/security/evidence.json"))
    p.add_argument("--out",default=str(ROOT/"build/security-report.json"))
    a=p.parse_args()
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
    if a.mode in ("baseline","release"):
        report["suites"]=run_suites(a.exe,out.parent/"security-logs",a.timeout)
        if any(x["status"]!="PASS" for x in report["suites"]):
            report["errors"].append("Baseline suites not all PASS")
    if a.mode in ("inventory","release"):
        requirements,problems=inventory(a.asvs_csv,a.wstg_ids,a.lock)
        report["errors"]+=problems
        problems,stats=check_coverage(requirements,a.coverage,a.evidence,case_ids,commit,binarysha)
        report["errors"]+=problems
        report["coverage"]=stats
        if a.mode=="release" and not binarysha:
            report["errors"].append("Release candidate binary hash missing")
    out.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print("CYBOU security gate:", "PASS" if not report["errors"] else "BLOCKED",
          "|",len(report["errors"]),"issues | report:",out)
    for error in report["errors"][:30]: print(" -",error,file=sys.stderr)
    if len(report["errors"])>30: print("...more in JSON report",file=sys.stderr)
    return 0 if not report["errors"] else 2

if __name__=="__main__":
    sys.exit(main())
