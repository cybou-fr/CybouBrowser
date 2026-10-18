#!/usr/bin/env python3
"""Lint: dynamic text shown by QML Label/Text items must declare textFormat: Text.PlainText.

Qt's default (AutoText) renders strings that look like HTML as rich text, and rich text can fetch
<img src> URLs from the application's own network stack - outside the browser profile, request
interceptor, HTTPS-only policy and tracker blocking. Page titles, URLs, file names and origins are
attacker-controlled, so every non-literal text binding is required to opt out explicitly.
Standard library only.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ITEM = re.compile(r'\b(Label|Text)\s*\{')
LITERAL = re.compile(r'^(?:qsTr\(\s*"(?:[^"\\]|\\.)*"\s*\)|"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\')$')


def blocks(source):
    """Yield (start_offset, body) for every Label/Text item, braces balanced, strings skipped."""
    for match in ITEM.finditer(source):
        depth, i, quote = 1, match.end(), None
        while i < len(source) and depth:
            ch = source[i]
            if quote:
                if ch == '\\':
                    i += 1
                elif ch == quote:
                    quote = None
            elif ch in '"\'':
                quote = ch
            elif ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
            i += 1
        yield match.start(), source[match.end():i - 1]


def top_level_text(body):
    """Return the expression of this item's own `text:` property (not nested items'), or None."""
    depth, i, quote, n = 0, 0, None, len(body)
    while i < n:
        ch = body[i]
        if quote:
            if ch == '\\':
                i += 1
            elif ch == quote:
                quote = None
        elif ch in '"\'':
            quote = ch
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        elif depth == 0 and body.startswith('text:', i) and (i == 0 or not (body[i - 1].isalnum() or body[i - 1] in '._')):
            j, d, q = i + 5, 0, None
            while j < n:
                c = body[j]
                if q:
                    if c == '\\':
                        j += 1
                    elif c == q:
                        q = None
                elif c in '"\'':
                    q = c
                elif c in '([{':
                    d += 1
                elif c in ')]}':
                    d -= 1
                elif d == 0 and c in ';\n':
                    break
                j += 1
            return body[i + 5:j].strip()
        i += 1
    return None


def top_level_has(body, token):
    depth, i, quote = 0, 0, None
    while i < len(body):
        ch = body[i]
        if quote:
            if ch == '\\':
                i += 1
            elif ch == quote:
                quote = None
        elif ch in '"\'':
            quote = ch
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        elif depth == 0 and body.startswith(token, i):
            return True
        i += 1
    return False


def lint(root=ROOT / 'qml'):
    findings = []
    for path in sorted(Path(root).glob('*.qml')):
        source = path.read_text(encoding='utf-8')
        for start, body in blocks(source):
            expression = top_level_text(body)
            if expression is None or LITERAL.match(expression):
                continue
            if top_level_has(body, 'textFormat:') and 'PlainText' in body[body.index('textFormat:'):body.index('textFormat:') + 40]:
                continue
            line = source.count('\n', 0, start) + 1
            findings.append((path.name, line, expression[:90]))
    return findings


def main():
    findings = lint()
    for name, line, expression in findings:
        print(f'{name}:{line}: dynamic text without textFormat: Text.PlainText: {expression}')
    print(f'QML_TEXT_LINT {"FAIL" if findings else "PASS"}: {len(findings)} finding(s)')
    return 1 if findings else 0


if __name__ == '__main__':
    sys.exit(main())
