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
