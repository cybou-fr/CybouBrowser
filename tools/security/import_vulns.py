#!/usr/bin/env python3
"""Normalize downloaded vulnerability feeds into tests/security/vulns/ for the offline risk gate.
Python 3.10+, standard library only. It never downloads: fetch the feeds yourself, then import them.

  python tools/security/import_vulns.py --kev known_exploited_vulnerabilities.json
  python tools/security/import_vulns.py --nvd nvd-chrome.json --cpe google:chrome=chromium
  python tools/security/import_vulns.py --osv osv-records.json --component qt

With --sbom, the complete snapshot goes to snapshot.full.json (not versioned, see .gitignore) and
snapshot.json keeps only the records that match a component version in that SBOM.

Feeds: CISA KEV https://www.cisa.gov/known-exploited-vulnerabilities-catalog (JSON feed),
NVD CVE API 2.0 https://services.nvd.nist.gov/rest/json/cves/2.0, OSV https://osv.dev.
Chrome version ranges do not account for security fixes Qt backports into its Chromium base:
a match is triaged in triage.json (for example status "fixed" with the Qt release that carries the fix).
"""
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent))
from owasp_gate import COMPONENTS, affects, sbom_versions

ROOT=Path(__file__).resolve().parents[2]

def read(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))

def write(path, data):
    Path(path).write_text(json.dumps(data,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")

def source(path):
    return {"file":Path(path).name,"sha256":hashlib.sha256(Path(path).read_bytes()).hexdigest()}

def from_nvd(data, cpes):
    records=[]
    for item in data.get("vulnerabilities",[]):
        cve=item.get("cve",{})
        for comp,ranges in nvd_ranges(cve,cpes).items():
            summary=next((d["value"] for d in cve.get("descriptions",[]) if d.get("lang")=="en"),"")
            records.append({"id":cve.get("id"),"aliases":[],"component":comp,"ranges":ranges,"summary":summary[:300]})
    return records

def nvd_ranges(cve, cpes):
    found={}
    for conf in cve.get("configurations",[]):
        for node in conf.get("nodes",[]):
            for m in node.get("cpeMatch",[]):
                parts=m.get("criteria","").split(":")
                comp=cpes.get(":".join(parts[3:5])) if len(parts)>5 and m.get("vulnerable") else None
                if not comp: continue
                r={"introduced":m.get("versionStartIncluding","0")}
                if m.get("versionEndExcluding"): r["fixed"]=m["versionEndExcluding"]
                elif m.get("versionEndIncluding"): r["lastAffected"]=m["versionEndIncluding"]
                elif parts[5] not in ("*","-"): r={"introduced":parts[5],"lastAffected":parts[5]}
                else: continue
                found.setdefault(comp,[]).append(r)
    return found

def from_osv(data, component):
    records=[]
    for rec in data.get("vulns",[data] if "id" in data else []):
        ranges=[]; versions=[]
        for aff in rec.get("affected",[]):
            versions+=aff.get("versions",[])
            for r in aff.get("ranges",[]):
                if r.get("type")=="GIT": continue
                cur={}
                for e in r.get("events",[]):
                    if "introduced" in e: cur={"introduced":e["introduced"]}
                    elif "fixed" in e or "last_affected" in e:
                        cur["fixed" if "fixed" in e else "lastAffected"]=e.get("fixed",e.get("last_affected"))
                        ranges.append(cur); cur={}
                if cur: ranges.append(cur)
        records.append({"id":rec.get("id"),"aliases":rec.get("aliases",[]),"component":component,
                        "versions":versions,"ranges":ranges,"summary":rec.get("summary","")[:300]})
    return records

def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--kev"); p.add_argument("--nvd"); p.add_argument("--osv")
    p.add_argument("--cpe",action="append",default=[],help="vendor:product=component, e.g. google:chrome=chromium")
    p.add_argument("--component",help="component name for --osv records")
    p.add_argument("--sbom",help="keep only records matching this CycloneDX SBOM in snapshot.json")
    p.add_argument("--retrieved-on",default=dt.date.today().isoformat())
    p.add_argument("--out",default=str(ROOT/"tests/security/vulns"))
    a=p.parse_args()
    out=Path(a.out); out.mkdir(parents=True,exist_ok=True)
    if a.kev:
        ids=sorted({v["cveID"] for v in read(a.kev).get("vulnerabilities",[]) if v.get("cveID")})
        write(out/"kev.json",{"schemaVersion":1,"retrievedOn":a.retrieved_on,"source":source(a.kev),"cveIds":ids})
        print("kev.json:",len(ids),"CVE IDs")
    new=[]
    if a.nvd:
        new+=from_nvd(read(a.nvd),dict(x.split("=",1) for x in a.cpe))
    if a.osv:
        if not a.component: p.error("--osv needs --component")
        new+=from_osv(read(a.osv),a.component)
    if a.nvd or a.osv:
        snap_file=out/("snapshot.full.json" if a.sbom else "snapshot.json")
        snap=read(snap_file) if snap_file.exists() else {"schemaVersion":1,"records":[],"sources":[]}
        replaced={(r["id"],r["component"]) for r in new}
        snap["records"]=sorted([r for r in snap.get("records",[]) if (r["id"],r["component"]) not in replaced]+new,
                               key=lambda r:(r["component"],r["id"]))
        snap["sources"]=snap.get("sources",[])+[source(x) for x in (a.nvd,a.osv) if x]
        snap["retrievedOn"]=a.retrieved_on
        write(snap_file,snap)
        print(snap_file.name+":",len(new),"records imported,",len(snap["records"]),"total")
        if a.sbom:
            versions=sbom_versions(a.sbom)
            matched=[r for r in snap["records"] if r["component"] in COMPONENTS
                     and versions.get(COMPONENTS[r["component"]]) and affects(r,versions[COMPONENTS[r["component"]]])]
            write(out/"snapshot.json",{**snap,"records":matched,"filteredBy":source(a.sbom)})
            print("snapshot.json:",len(matched),"records match the SBOM")

if __name__=="__main__":
    main()
