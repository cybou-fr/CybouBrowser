#!/usr/bin/env python3\n"""Self-tests for fail-closed release gate. Standard library only."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import subprocess
sys.path.insert(0,str(Path(__file__).resolve().parent))
import owasp_gate as gate

class GateContractTests(unittest.TestCase):
    def test_missing_standards_blocks(self):
        req,errors=gate.inventory(None,None,"unused")
        self.assertFalse(req)
        self.assertTrue(errors)

    def pinned_inventory(self, lock=None, asvs=None, wstg=None):
        root=gate.ROOT/"tests/security"
        return gate.inventory(asvs or root/"standards/asvs-5.0.0.csv",
                              wstg or root/"standards/wstg-4.2-ids.txt",
                              lock or root/"standards.lock.json")

    def test_complete_pinned_inventory(self):
        requirements,errors=self.pinned_inventory()
        self.assertEqual(errors,[])
        self.assertEqual(sum(x.startswith("ASVS:") for x in requirements),345)
        self.assertEqual(sum(x.startswith("WSTG:") for x in requirements),97)
        coverage=gate.read_json(gate.ROOT/"tests/security/coverage.json")["requirements"]
        self.assertEqual({x["id"] for x in coverage},requirements)
        self.assertEqual(len(coverage),len(requirements))

    def test_tampered_inventory_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            tampered=Path(d)/"asvs.csv"
            tampered.write_text("req_id\nV1.1.1\n",encoding="utf-8")
            requirements,errors=self.pinned_inventory(asvs=tampered)
            self.assertFalse(requirements)
            self.assertTrue(any("SHA-256 mismatch" in x for x in errors))

    def test_repinning_duplicate_or_malformed_id_still_fails(self):
        root=gate.ROOT/"tests/security"
        original=(root/"standards/asvs-5.0.0.csv").read_bytes()
        for extra in [b"V1.1.1\n",b"garbage\n"]:
            with self.subTest(extra=extra),tempfile.TemporaryDirectory() as d:
                base=Path(d); data=base/"asvs.csv"; data.write_bytes(original+b"\n"+extra)
                lock=gate.read_json(root/"standards.lock.json")
                lock["asvs"]["sha256"]=gate.sha(data)
                lockfile=base/"lock.json"; lockfile.write_text(json.dumps(lock),encoding="utf-8")
                _,errors=self.pinned_inventory(lock=lockfile,asvs=data)
                self.assertTrue(any("Duplicate ASVS" in x or "Malformed ASVS" in x for x in errors))

    def test_wrong_pinned_count_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            lock=gate.read_json(gate.ROOT/"tests/security/standards.lock.json")
            lock["asvs"]["requirementCount"]=344
            path=Path(d)/"lock.json"; path.write_text(json.dumps(lock),encoding="utf-8")
            _,errors=self.pinned_inventory(lock=path)
            self.assertTrue(any("count mismatch" in x for x in errors))

    def test_missing_binary_blocks(self):
        with tempfile.TemporaryDirectory() as d:
            results=gate.run_suites(None,Path(d),1)
        self.assertEqual(len(results),len(gate.SUITES))
        self.assertTrue(all(x["status"]=="BLOCKED" for x in results))

    def test_zero_exit_without_diagnostics_fails(self):
        with tempfile.TemporaryDirectory() as d:
            with patch.object(gate.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
                results=gate.run_suites(Path(__file__),Path(d),1)
            self.assertTrue(all(x["status"]=="FAIL" and x["logBytes"]==0 for x in results))

    def test_success_preserves_suite_diagnostics(self):
        def execute(*args, **kwargs):
            kwargs["stdout"].write("fixture suite PASS\n")
            return subprocess.CompletedProcess([], 0)
        with tempfile.TemporaryDirectory() as d:
            with patch.object(gate.subprocess, "run", side_effect=execute):
                results=gate.run_suites(Path(__file__),Path(d),1)
            self.assertTrue(all(x["status"]=="PASS" and x["logBytes"]>0 for x in results))
            self.assertTrue(all(x["sha256"]==gate.sha(x["log"]) for x in results))

    def test_untriaged_and_planned_block(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"coverage.json").write_text('{"requirements":[]}',encoding="utf-8")
            errors,s=gate.check_coverage({"ASVS:v5.0.0-1.2.5"},base/"coverage.json",
                base/"missing-evidence.json",
                {"CYB-PERM-001":{"status":"planned"}},"example-commit","binary-sha")
            self.assertEqual(s["untriaged"],1)
            self.assertTrue(any("not verified" in e for e in errors))

    def test_evidence_is_bound_to_nonempty_unchanged_log_and_candidate(self):
        with tempfile.TemporaryDirectory() as d:
            log=Path(d)/"case.log"; log.write_text("negative fixture PASS\n",encoding="utf-8")
            item={"caseId":"TEST-001","result":"PASS","commit":"source-a",
                  "binarySha256":"binary-a","log":str(log),"logSha256":gate.sha(log)}
            self.assertTrue(gate.current_pass_evidence(item,"source-a","binary-a"))
            for commit,binary in [("unknown","binary-a"),("source-b","binary-a"),("source-a",""),("source-a","binary-b")]:
                self.assertFalse(gate.current_pass_evidence(item,commit,binary))
            log.write_text("changed\n",encoding="utf-8")
            self.assertFalse(gate.current_pass_evidence(item,"source-a","binary-a"))
            log.write_text("",encoding="utf-8"); item["logSha256"]=gate.sha(log)
            self.assertFalse(gate.current_pass_evidence(item,"source-a","binary-a"))
            log.unlink()
            self.assertFalse(gate.current_pass_evidence(item,"source-a","binary-a"))

    def test_applicable_control_requires_real_case_evidence(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d); control="ASVS:v5.0.0-8.2.1"
            coverage=base/"coverage.json"; evidence=base/"evidence.json"; log=base/"case.log"
            coverage.write_text(json.dumps({"requirements":[{"id":control,"disposition":"applicable",
                                 "owner":"test-owner","caseIds":["TEST-001"],"verificationStatus":"verified",
                                 "assertions":["TEST-001: rejects stale token"]}]}),encoding="utf-8")
            log.write_text("PASS\n",encoding="utf-8")
            item={"caseId":"TEST-001","result":"PASS","commit":"source-a","binarySha256":"binary-a",
                  "log":str(log),"logSha256":gate.sha(log)}
            cases={"TEST-001":{"status":"verified"}}
            def check(items):
                evidence.write_text(json.dumps({"evidence":items}),encoding="utf-8")
                return gate.check_coverage({control},coverage,evidence,cases,"source-a","binary-a")[0]
            self.assertEqual(check([item]),[])
            self.assertTrue(any("Duplicate" in x for x in check([item,item])))
            item["logSha256"]="wrong"
            errors=check([item])
            self.assertTrue(any("current-sha/hash" in x for x in errors))
            self.assertTrue(any("current-revision" in x for x in errors))

    def test_reviewed_na_does_not_waive_native_tests(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"coverage.json").write_text(json.dumps({"requirements":[{
                "id":"ASVS:v5.0.0-1.2.5","disposition":"not_applicable",
                "rationale":"No backend OS command execution in scoped app",
                "reviewedBy":"security-reviewer"}]}),encoding="utf-8")
            errors,s=gate.check_coverage({"ASVS:v5.0.0-1.2.5"},base/"coverage.json",
                base/"missing-evidence.json",
                {"CYB-PERM-001":{"status":"planned"}},"example-commit","binary-sha")
            self.assertEqual(s["notApplicable"],1)
            self.assertTrue(errors)

    def coverage_errors(self, row, reviewers=frozenset()):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d); log=base/"case.log"; log.write_text("PASS\n",encoding="utf-8")
            (base/"coverage.json").write_text(json.dumps({"requirements":[row]}),encoding="utf-8")
            (base/"evidence.json").write_text(json.dumps({"evidence":[{"caseId":"TEST-001","result":"PASS",
                "commit":"source-a","binarySha256":"binary-a","log":str(log),"logSha256":gate.sha(log)}]}),encoding="utf-8")
            return gate.check_coverage({row["id"]},base/"coverage.json",base/"evidence.json",
                {"TEST-001":{"status":"verified"}},"source-a","binary-a",reviewers)[0]

    def test_applicable_pass_log_without_completed_verification_blocks(self):
        row={"id":"ASVS:v5.0.0-8.2.1","disposition":"applicable","owner":"o","caseIds":["TEST-001"]}
        for extra in [{"verificationStatus":"not_verified","assertions":["a"]},{"verificationStatus":"verified"}]:
            with self.subTest(extra=extra):
                self.assertTrue(any("not verified or no mapped assertions" in e for e in self.coverage_errors({**row,**extra})))

    def test_na_needs_listed_reviewer_date_reference_and_specific_reason(self):
        row={"id":"ASVS:v5.0.0-13.4.3","disposition":"not_applicable","rationale":"No CYBOU web server",
             "reviewedBy":"Jane Reviewer","reviewedOn":"2026-10-22","reviewRef":"commit abc123"}
        self.assertEqual(self.coverage_errors(row,{"Jane Reviewer"}),[])
        self.assertTrue(any("not listed" in e for e in self.coverage_errors(row)))
        for change in [{"reviewedOn":"yesterday"},{"reviewRef":""},{"proposalRationale":"No CYBOU web server"}]:
            with self.subTest(change=change):
                self.assertTrue(self.coverage_errors({**row,**change},{"Jane Reviewer"}))

    def test_framework_gate_passes_on_repository_catalogs(self):
        requirements,errors=self.pinned_inventory()
        cases,_,_=gate.case_index(gate.ROOT/"tests/security/cases.json")
        errors,stats=gate.check_framework(requirements,gate.ROOT/"tests/security/coverage.json",
                                         cases,gate.ROOT/"tests/security/threats.json")
        self.assertEqual(errors,[])
        self.assertGreater(stats["threats"],0)

    def test_framework_gate_rejects_broken_threat_links(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"coverage.json").write_text(json.dumps({"requirements":[{"id":"ASVS:v5.0.0-1.1.1","disposition":"untriaged"}]}),encoding="utf-8")
            (base/"threats.json").write_text(json.dumps({"threats":[{"id":"CWE-416","title":"t","caseIds":["NOPE"]},
                                                                     {"id":"bad","title":"t","caseIds":["TEST-001"]}]}),encoding="utf-8")
            errors,_=gate.check_framework({"ASVS:v5.0.0-1.1.1"},base/"coverage.json",{"TEST-001":{}},base/"threats.json")
            for text in ["needs a recorded proposal","unknown case NOPE","malformed or duplicate threat id: bad"]:
                self.assertTrue(any(text in e for e in errors),text)

    def risk_errors(self, records, decisions, kev=(), retrieved="2026-10-20", strict=False):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"sbom.json").write_text(json.dumps({"components":[{"bom-ref":"qt-webengine-chromium","version":"6.12.0",
                "properties":[{"name":"cybou:chromium-version","value":"140.0.7339.225"}]}]}),encoding="utf-8")
            (base/"snapshot.json").write_text(json.dumps({"retrievedOn":retrieved,"records":records,
                "contentSha256":gate.canonical_sha(records)}),encoding="utf-8")
            (base/"kev.json").write_text(json.dumps({"retrievedOn":retrieved,"cveIds":list(kev),
                "contentSha256":gate.canonical_sha(list(kev))}),encoding="utf-8")
            (base/"triage.json").write_text(json.dumps({"decisions":decisions}),encoding="utf-8")
            return gate.check_risk(base/"sbom.json",base,gate.dt.date(2026,10,22),strict=strict)

    def test_risk_gate_matches_sbom_versions_and_requires_triage(self):
        hit={"id":"CVE-2099-0001","component":"chromium","ranges":[{"introduced":"0","fixed":"141.0.7390.54"}]}
        miss={"id":"CVE-2099-0002","component":"chromium","ranges":[{"introduced":"0","fixed":"139.0.1"}]}
        errors,stats=self.risk_errors([hit,miss],[])
        self.assertEqual(stats["matches"],1)
        self.assertTrue(any("not triaged" in e for e in errors))
        fixed={"vulnId":"CVE-2099-0001","component":"chromium","status":"fixed","justification":"backported in Qt 6.12.0",
               "method":"backport-commit","evidence":["https://example.invalid/commit"]}
        self.assertEqual(self.risk_errors([hit,miss],[fixed])[0],[])
        errors,stats=self.risk_errors([hit],[{**fixed,"status":"affected"}],kev=["CVE-2099-0001"])
        self.assertTrue(any("CISA KEV" in e for e in errors))
        self.assertEqual(stats["kev"],1)
        self.assertTrue(any("not_affected needs" in e for e in self.risk_errors([hit],[{**fixed,"status":"not_affected","justification":""}])[0]))

    def patch_level_stats(self, fixed, backported, decisions=()):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"sbom.json").write_text(json.dumps({"components":[{"bom-ref":"qt-webengine-chromium","properties":[
                {"name":"cybou:chromium-version","value":"140.0.7339.225"},
                {"name":"cybou:chromium-security-patch-version","value":"152.0.7977.83"}]}]}),encoding="utf-8")
            rec={"id":"CVE-2099-0006","component":"chromium","ranges":[{"introduced":"0","fixed":fixed}]}
            (base/"snapshot.json").write_text(json.dumps({"retrievedOn":"2026-10-21","records":[rec],
                "contentSha256":gate.canonical_sha([rec])}),encoding="utf-8")
            (base/"kev.json").write_text(json.dumps({"retrievedOn":"2026-10-21","cveIds":[],"contentSha256":gate.canonical_sha([])}),encoding="utf-8")
            (base/"triage.json").write_text(json.dumps({"decisions":list(decisions)}),encoding="utf-8")
            cves={"CVE-2099-0006":["abc"]} if backported else {}
            (base/"qt-backports.json").write_text(json.dumps({"cves":cves,"contentSha256":gate.canonical_sha(cves)}),encoding="utf-8")
            return gate.check_risk(base/"sbom.json",base,gate.dt.date(2026,10,22))

    def test_patch_level_needs_both_declared_level_and_backport_commit(self):
        errors,stats=self.patch_level_stats("152.0.7977.82",True)
        self.assertEqual((errors,stats["patchLevel"],stats["open"]),([],1,0))
        for fixed,backported in [("152.0.7977.82",False),("153.0.8010.36",True)]:
            with self.subTest(fixed=fixed,backported=backported):
                errors,stats=self.patch_level_stats(fixed,backported)
                self.assertEqual((stats["patchLevel"],stats["open"]),(0,1))
                self.assertTrue(any("not triaged" in e for e in errors))
        affected={"vulnId":"CVE-2099-0006","component":"chromium","status":"affected"}
        errors,stats=self.patch_level_stats("152.0.7977.82",True,[affected])
        self.assertTrue(any("known vulnerability affects" in e for e in errors))

    def test_reverted_backport_does_not_count(self):
        import qt_backports as qb
        found=qb.index([{"title":"[backport] CVE-2099-0007 (1/2)","sha":"a"},{"title":"[backport] CVE-2099-0007 (2/2)","sha":"b"},
                        {"title":'Revert "[backport] CVE-2099-0007 (1/2)"',"sha":"c"},{"title":"[Backport] CVE-2099-0008","sha":"d"},
                        {"title":'Revert "[Backport] CVE-2099-0008"',"sha":"e"},{"title":"[Backport] Dependency for CVE-2099-0009","sha":"f"}])
        self.assertEqual(found,{"CVE-2099-0007":["b"]})

    def test_unsupported_or_weak_decisions_block(self):
        hit={"id":"CVE-2099-0001","component":"chromium","ranges":[{"introduced":"0","fixed":"141.0.1"}]}
        bare={"vulnId":"CVE-2099-0001","component":"chromium","status":"fixed","justification":"trust me"}
        self.assertTrue(any("needs a method" in e for e in self.risk_errors([hit],[bare])[0]))
        branch={**bare,"method":"backport-commit","evidence":["https://example.invalid/c"]}
        self.assertEqual(self.risk_errors([hit],[branch])[0],[])
        self.assertTrue(any("release needs binary or source" in e for e in self.risk_errors([hit],[branch],strict=True)[0]))
        tested={**branch,"method":"binary-test"}
        errors,stats=self.risk_errors([hit],[tested],strict=True)
        self.assertEqual((errors,stats["byEvidence"]["binary"]),([],1))

    def test_source_inspection_is_bound_to_shipped_release_and_file_hashes(self):
        versions={("qt-webengine-chromium",None):"6.140.0",("qt-webengine-chromium","cybou:chromium-security-patch-version"):"152.0.7977.83"}
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"qtwebengine"; f=src/"src/3rdparty/chromium/v8/a.cc"; f.parent.mkdir(parents=True)
            f.write_text("code\n",encoding="utf-8")
            (src/".cmake.conf").write_text('set(QT_REPO_MODULE_VERSION "6.140.0")\n',encoding="utf-8")
            (src/"CHROMIUM_VERSION").write_text("Patched with security patches up to Chromium version: 152.0.7977.83\n",encoding="utf-8")
            good={"sourceBinding":{"webengine":"6.140.0","patchLevel":"152.0.7977.83","files":[{"path":"v8/a.cc","sha256":gate.sha(f)}]}}
            self.assertEqual(gate.source_binding_errors("X",good,versions,src),([],True))
            self.assertEqual(gate.source_binding_errors("X",good,versions,Path(d)/"absent"),([],False))
            other={"sourceBinding":{**good["sourceBinding"],"webengine":"6.139.1"}}
            self.assertTrue(gate.source_binding_errors("X",other,versions,src)[0])
            f.write_text("patched\n",encoding="utf-8")
            errors,verified=gate.source_binding_errors("X",good,versions,src)
            self.assertFalse(verified); self.assertTrue(any("differs" in e for e in errors))
            self.assertTrue(gate.source_binding_errors("X",{},versions,src)[0])

    def test_runtime_engine_versions_must_match_sbom(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d); log=base/"s.log"; sbom=base/"sbom.json"
            sbom.write_text(json.dumps({"components":[{"bom-ref":"qt","version":"6.12.0"},
                {"bom-ref":"qt-webengine-chromium","version":"6.140.0","properties":[
                 {"name":"cybou:chromium-version","value":"140.0.7339.225"},
                 {"name":"cybou:chromium-security-patch-version","value":"152.0.7977.83"}]}]}),encoding="utf-8")
            log.write_text('ENGINE 6.12.0 6.140.0 140.0.7339.225 security patch 152.0.7977.83\n',encoding="utf-8")
            errors,runtime=gate.engine_runtime_errors([{"log":str(log)}],sbom)
            self.assertEqual((errors,runtime["patch"]),([],"152.0.7977.83"))
            log.write_text('ENGINE "6.12.0" "6.140.0" "140.0.7339.264" "security patch" "152.0.7977.83"\n',encoding="utf-8")
            self.assertEqual(gate.engine_runtime_errors([{"log":str(log)}],sbom)[0],["runtime chromium 140.0.7339.264 differs from SBOM 140.0.7339.225"])
            log.write_text("no engine line\n",encoding="utf-8")
            self.assertTrue(gate.engine_runtime_errors([{"log":str(log)}],sbom)[0])

    def test_product_suites_must_run_with_product_protections(self):
        with tempfile.TemporaryDirectory() as d:
            def suite(name, text):
                log=Path(d)/(name+".log"); log.write_text(text,encoding="utf-8"); return {"suite":name,"status":"PASS","log":str(log)}
            good="ENGINE_CONFIG strictWebRtc=1 disableWasm=1 pna=1 unsafe=none\n"
            errors,seen=gate.engine_config_errors([suite("security",good),suite("webrtc","no line: own children\n")])
            self.assertEqual((errors,list(seen)),([],["security"]))
            for text,expected in [("ENGINE_CONFIG strictWebRtc=1 disableWasm=0 pna=1 unsafe=none\n","disableWasm"),
                                  ("ENGINE_CONFIG strictWebRtc=0 disableWasm=1 pna=1 unsafe=none\n","strictWebRtc"),
                                  ("ENGINE_CONFIG strictWebRtc=1 disableWasm=1 pna=1 unsafe=--no-sandbox\n","--no-sandbox"),
                                  ("ENGINE 6.12.0\n","no ENGINE_CONFIG")]:
                with self.subTest(expected=expected):
                    errors,_=gate.engine_config_errors([suite("smoke",text)])
                    self.assertTrue(any(expected in e for e in errors),errors)

    def test_feed_edited_after_import_is_detected(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"sbom.json").write_text(json.dumps({"components":[]}),encoding="utf-8")
            (base/"snapshot.json").write_text(json.dumps({"retrievedOn":"2026-10-21","records":[],
                "contentSha256":gate.canonical_sha([{"id":"CVE-2099-0001"}])}),encoding="utf-8")
            (base/"kev.json").write_text(json.dumps({"retrievedOn":"2026-10-21","cveIds":["CVE-1"],"contentSha256":"0"}),encoding="utf-8")
            errors,_=gate.check_risk(base/"sbom.json",base,gate.dt.date(2026,10,22))
            self.assertEqual(sum("does not match contentSha256" in e for e in errors),2)

    def test_framework_requires_existing_test_implementations(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"coverage.json").write_text(json.dumps({"requirements":[]}),encoding="utf-8")
            (base/"threats.json").write_text(json.dumps({"threats":[{"id":"CWE-20","title":"t","caseIds":["A"]}]}),encoding="utf-8")
            cases={"A":{"status":"in_progress","kind":"execution"},"B":{"status":"planned","kind":"execution","testSources":["src/does-not-exist.h"]},
                   "C":{"status":"in_progress","kind":"execution","testSources":["tools/security/owasp_gate.py"]},"D":{"status":"planned","kind":"audit"}}
            errors,_=gate.check_framework(set(),base/"coverage.json",cases,base/"threats.json")
            self.assertIn("A: in_progress case lists no testSources",errors)
            self.assertIn("B: test source missing: src/does-not-exist.h",errors)
            self.assertFalse(any(e.startswith("C:") for e in errors))
            self.assertTrue(any(e.startswith("D: kind must be") for e in errors))

    def test_na_report_facts_hold_and_report_is_current(self):
        import na_report
        text,errors=na_report.build()
        self.assertEqual(errors,[])
        self.assertEqual(na_report.REPORT.read_text(encoding="utf-8"),text)

    def test_na_report_detects_broken_fact_and_bad_links(self):
        import na_report
        data=gate.read_json(na_report.CATEGORIES)
        data["facts"]["no-dynamic-regex"]["forbid"]="QRegularExpression"   # present in product sources
        hits,_=na_report.check_facts(data)
        self.assertTrue(hits["no-dynamic-regex"])
        data["categories"][0]["equivalentCases"].append("CYB-NOPE")
        data["categories"][0]["architecture"].append("docs/missing.md")
        rows=[{"id":"X","disposition":"untriaged","proposedDisposition":"not_applicable","proposalRationale":"unknown"}]
        errors=na_report.validate(data,rows,{c:{} for c in ("CYB-HTTP-001","CYB-HTTP-002","CYB-ORIGIN-001","CYB-INPUT-002")})
        for text in ["X: N/A proposal has no category","unknown case CYB-NOPE","architecture file missing: docs/missing.md"]:
            self.assertTrue(any(text in e for e in errors),text)

    def test_risk_gate_blocks_stale_or_missing_feeds(self):
        self.assertTrue(any("days old" in e for e in self.risk_errors([],[],retrieved="2026-09-01")[0]))
        with tempfile.TemporaryDirectory() as d:
            self.assertTrue(gate.check_risk(Path(d)/"none.json",d,gate.dt.date(2026,10,22))[0])

    def test_nvd_and_osv_records_normalize_to_gate_ranges(self):
        import import_vulns as imp
        nvd={"vulnerabilities":[{"cve":{"id":"CVE-2099-0003","descriptions":[{"lang":"en","value":"x"}],
             "configurations":[{"nodes":[{"cpeMatch":[{"vulnerable":True,"criteria":"cpe:2.3:a:google:chrome:*:*:*:*:*:*:*:*",
             "versionEndExcluding":"141.0.7390.54"}]}]}]}}]}
        rec=imp.from_nvd(nvd,{"google:chrome":"chromium"})[0]
        self.assertTrue(gate.affects(rec,"140.0.7339.225"))
        self.assertFalse(gate.affects(rec,"141.0.7390.54"))
        osv={"id":"OSV-1","affected":[{"ranges":[{"type":"ECOSYSTEM","events":[{"introduced":"6.0"},{"last_affected":"6.12.0"}]}]}]}
        rec=imp.from_osv(osv,"qt")[0]
        self.assertTrue(gate.affects(rec,"6.12.0"))
        self.assertFalse(gate.affects(rec,"6.12.1"))

    def test_import_with_sbom_versions_only_matching_records(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"sbom.json").write_text(json.dumps({"components":[{"bom-ref":"qt-webengine-chromium",
                "properties":[{"name":"cybou:chromium-version","value":"140.0.7339.225"}]}]}),encoding="utf-8")
            cpe="cpe:2.3:a:google:chrome:*:*:*:*:*:*:*:*"
            nvd={"vulnerabilities":[{"cve":{"id":cid,"configurations":[{"nodes":[{"cpeMatch":[{"vulnerable":True,
                 "criteria":cpe,"versionEndExcluding":fixed}]}]}]}} for cid,fixed in [("CVE-2099-0004","141.0.1"),("CVE-2099-0005","139.0.1")]]}
            (base/"nvd.json").write_text(json.dumps(nvd),encoding="utf-8")
            argv=["import_vulns.py","--nvd",str(base/"nvd.json"),"--cpe","google:chrome=chromium",
                  "--sbom",str(base/"sbom.json"),"--out",str(base/"vulns")]
            import import_vulns as imp
            with patch.object(sys,"argv",argv): imp.main()
            self.assertEqual(len(gate.read_json(base/"vulns/snapshot.full.json")["records"]),2)
            self.assertEqual([r["id"] for r in gate.read_json(base/"vulns/snapshot.json")["records"]],["CVE-2099-0004"])

if __name__=="__main__":
    unittest.main()
