#!/usr/bin/env python3
"""Index the CVE backports Qt cherry-picked into its Chromium fork, for the risk gate's patch-level status.
Python 3.10+, standard library only. Online only when fetching; the gate reads the resulting file offline.

  python tools/security/qt_backports.py --until 2026-09-28T10:05:00Z
  python tools/security/qt_backports.py --commits saved-commits.json --until 2026-09-28T10:05:00Z

--until is the build time of the shipped Qt WebEngine: later commits are not in the binary.
A CVE counts as backported when a "[Backport] CVE-..." commit exists whose title was not reverted;
"Dependency for CVE-..." commits alone do not count.
"""
import argparse
import json
import os
from pathlib import Path
import re
import sys
import urllib.request

sys.path.insert(0,str(Path(__file__).resolve().parent))
from owasp_gate import canonical_sha

ROOT=Path(__file__).resolve().parents[2]
API="https://api.github.com/repos/qt/qtwebengine-chromium/commits"
BACKPORT=re.compile(r"\[backport\]\s*(CVE-\d{4}-\d+)",re.I)
REVERT=re.compile(r'^Revert "(.+)"$')

def fetch(branch, until):
    commits=[]
    for page in range(1,200):
        url=f"{API}?sha={branch}&until={until}&per_page=100&page={page}"
        req=urllib.request.Request(url)
        # Anonymous GitHub API calls are limited to 60 per hour; a token (read-only, no scopes) raises that.
        if os.environ.get("GITHUB_TOKEN"): req.add_header("Authorization","Bearer "+os.environ["GITHUB_TOKEN"])
        batch=json.load(urllib.request.urlopen(req,timeout=60))
        if not batch: break
        commits+=[{"sha":c["sha"],"date":c["commit"]["committer"]["date"],"title":c["commit"]["message"].splitlines()[0]} for c in batch]
    return commits

def index(commits):
    reverted={m.group(1) for c in commits if (m:=REVERT.match(c["title"]))}
    found={}
    for c in commits:
        m=BACKPORT.search(c["title"])
        if m and not REVERT.match(c["title"]) and c["title"] not in reverted:
            found.setdefault(m.group(1).upper(),[]).append(c["sha"])
    return found

def main():
    p=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--branch",default="140-based")
    p.add_argument("--until",required=True,help="ISO time of the shipped Qt WebEngine build")
    p.add_argument("--commits",help="previously saved commit list (sha/date/title) instead of fetching")
    p.add_argument("--scanned",type=int,help="with --commits: total commits of the scan the list was taken from")
    p.add_argument("--out",default=str(ROOT/"tests/security/vulns/qt-backports.json"))
    a=p.parse_args()
    commits=json.loads(Path(a.commits).read_text(encoding="utf-8")) if a.commits else fetch(a.branch,a.until)
    commits=[c for c in commits if c["date"]<=a.until]
    found=index(commits)
    data={"schemaVersion":1,"repository":"https://github.com/qt/qtwebengine-chromium","branch":a.branch,
          "until":a.until,"commitsScanned":a.scanned or len(commits),
          "contentSha256":canonical_sha(found),"cves":{k:found[k] for k in sorted(found)}}
    Path(a.out).write_text(json.dumps(data,indent=1,ensure_ascii=False)+"\n",encoding="utf-8")
    print(Path(a.out).name+":",len(found),"backported CVEs from",len(commits),"commits")

if __name__=="__main__":
    main()
