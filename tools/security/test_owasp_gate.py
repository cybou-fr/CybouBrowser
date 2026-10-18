#!/usr/bin/env python3
"""Self-tests for fail-closed release gate. Standard library only."""
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
        self.assertEqual(len(results),11)
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
                                 "owner":"test-owner","caseIds":["TEST-001"]}]}),encoding="utf-8")
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

if __name__=="__main__":
    unittest.main()
