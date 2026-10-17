#!/usr/bin/env python3
"""Self-tests for fail-closed release gate. Standard library only."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parent))
import owasp_gate as gate

class GateContractTests(unittest.TestCase):
    def test_missing_standards_blocks(self):
        req,errors=gate.inventory(None,None,"unused")
        self.assertFalse(req)
        self.assertTrue(errors)

    def test_missing_binary_blocks(self):
        with tempfile.TemporaryDirectory() as d:
            results=gate.run_suites(None,Path(d),1)
        self.assertEqual(len(results),6)
        self.assertTrue(all(x["status"]=="BLOCKED" for x in results))

    def test_untriaged_and_planned_block(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)
            (base/"coverage.json").write_text('{"requirements":[]}',encoding="utf-8")
            errors,s=gate.check_coverage({"ASVS:v5.0.0-1.2.5"},base/"coverage.json",
                base/"missing-evidence.json",
                {"CYB-PERM-001":{"status":"planned"}},"example-commit","binary-sha")
            self.assertEqual(s["untriaged"],1)
            self.assertTrue(any("not verified" in e for e in errors))

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
