import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WEB_VIEW = re.compile(r"^\s*WebEngineView\s*\{", re.MULTILINE)
DISALLOW = re.compile(r"unknownUrlSchemePolicy:\s*WebEngineSettings\.DisallowUnknownUrlSchemes")
LOOSEN = re.compile(r"AllowAllUnknownUrlSchemes|AllowUnknownUrlSchemesFromUserInteraction")


def qml_files():
    return sorted((ROOT / "qml").glob("*.qml"))


class ProductWebSettings(unittest.TestCase):
    """Ties the engine behaviour proven by --protocol-test to the policy the product actually sets."""

    def test_every_web_view_disallows_unknown_url_schemes(self):
        views = [p for p in qml_files() if WEB_VIEW.search(p.read_text(encoding="utf-8"))]
        self.assertTrue(views, "no WebEngineView declaration found")
        for path in views:
            self.assertTrue(DISALLOW.search(path.read_text(encoding="utf-8")), f"{path.name} does not disallow unknown URL schemes")

    def test_no_file_loosens_the_scheme_policy(self):
        for path in qml_files():
            self.assertIsNone(LOOSEN.search(path.read_text(encoding="utf-8")), f"{path.name} loosens the scheme policy")

    def test_scripts_are_not_built_by_concatenating_into_string_literals(self):
        # runJavaScript arguments must embed values with JSON.stringify, never inside hand-written quotes.
        for path in qml_files():
            for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                self.assertNotRegex(line, r"""\(\s*'\s*"\s*\+|\(\s*"\s*'\s*\+""", f"{path.name}:{number} concatenates a value into a quoted JS argument")

    def test_javascript_uses_strict_equality(self):
        # ASVS 15.3.5: loose == / != allow type juggling in security decisions.
        string = re.compile(r"""("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')""")
        loose = re.compile(r"(?<![=!<>])==(?!=)|!=(?!=)")
        offenders = []
        for path in qml_files():
            in_block = False
            for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                code = string.sub('""', line)
                if in_block:
                    if "*/" not in code:
                        continue
                    code = code.split("*/", 1)[1]
                    in_block = False
                if "/*" in code:
                    code, rest = code.split("/*", 1)
                    in_block = "*/" not in rest
                code = code.split("//", 1)[0]
                if loose.search(code):
                    offenders.append(f"{path.name}:{number}")
        self.assertEqual(offenders, [], "loose equality found: " + ", ".join(offenders[:20]))

    def test_no_application_code_opens_non_local_urls_externally(self):
        # QDesktopServices::openUrl may only be fed local files/folders chosen by the application.
        sources = list((ROOT / "src").rglob("*.h")) + list((ROOT / "src").rglob("*.cpp"))
        checked = 0
        for path in sources:
            if "Tests" in path.name or "Probe" in path.name:
                continue
            for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
                if "QDesktopServices::openUrl" in line:
                    checked += 1
                    self.assertIn("fromLocalFile", line, f"{path.name}:{number} opens a non-local URL externally")
        self.assertGreater(checked, 0, "expected at least one openUrl call site to check")


if __name__ == "__main__":
    unittest.main()
