import tempfile
import unittest
from pathlib import Path

import qml_text_lint as lint


def findings_for(source):
    with tempfile.TemporaryDirectory() as tmp:
        (Path(tmp) / "Sample.qml").write_text(source, encoding="utf-8")
        return lint.lint(Path(tmp))


class QmlTextLint(unittest.TestCase):
    def test_product_qml_has_no_unqualified_dynamic_text(self):
        # Page titles, URLs, file names and origins reach these items; AutoText would render
        # "<img src=...>" as rich text and fetch it through the application's own network stack.
        self.assertEqual(lint.lint(), [])

    def test_dynamic_text_without_plain_text_is_reported(self):
        self.assertEqual(len(findings_for('Item { Label { text: modelData.title } }')), 1)

    def test_literals_and_plain_text_pass(self):
        self.assertEqual(findings_for('Item { Label { text: qsTr("Fixed") } Text { text: "x" } }'), [])
        self.assertEqual(findings_for('Item { Label { textFormat: Text.PlainText; text: modelData.title } }'), [])

    def test_multiline_object_and_nested_item_are_handled(self):
        multiline = 'Item {\n Label {\n  text: tab.title\n  color: "red"\n }\n}'
        self.assertEqual(len(findings_for(multiline)), 1)
        nested = 'Item { Label { textFormat: Text.PlainText; text: a.b; Text { text: c.d } } }'
        self.assertEqual(len(findings_for(nested)), 1)  # the nested item needs its own opt-out

    def test_script_assigned_text_requires_plain_text_on_the_target(self):
        bad = 'Item { Label { id: note } function show(v) { note.text = v } }'
        self.assertEqual(len(findings_for(bad)), 1)
        good = 'Item { Label { id: note; textFormat: Text.PlainText } function show(v) { note.text = v } }'
        self.assertEqual(findings_for(good), [])
        comparison = 'Item { Label { id: note } function same(v) { return note.text == v } }'
        self.assertEqual(findings_for(comparison), [])

    def test_rich_text_formats_do_not_satisfy_the_rule(self):
        self.assertEqual(len(findings_for('Item { Label { textFormat: Text.StyledText; text: a.b } }')), 1)


if __name__ == "__main__":
    unittest.main()
