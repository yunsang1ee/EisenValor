"""Sync the standalone keyboard viewer from keyboard_mapping.json."""

import argparse
import json
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check without writing files')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    mapping = json.loads((root / 'keyboard_mapping.json').read_text(encoding='utf-8'))
    html_path = root / 'keyboard_mapping.html'
    html = html_path.read_text(encoding='utf-8')
    pattern = re.compile(r'(<script type="application/json" id="embeddedMapping">)\s*(.*?)\s*(</script>)', re.S)
    matches = list(pattern.finditer(html))
    if len(matches) != 1:
        parser.error('Expected exactly one embeddedMapping JSON block')
    match = matches[0]
    if args.check:
        if json.loads(match.group(2)) != mapping:
            parser.error('HTML embedded mapping differs from keyboard_mapping.json; run sync_mapping.py')
        print('PASS: JSON and HTML embedded mapping match')
        return
    # Escape < to prevent a binding note from terminating the script element.
    data = json.dumps(mapping, ensure_ascii=True, indent=2).replace('<', '\\u003c')
    replacement = match.group(1) + '\n' + data + '\n  ' + match.group(3)
    updated = html[:match.start()] + replacement + html[match.end():]
    if updated != html:
        html_path.write_text(updated, encoding='utf-8')
    print('Synced keyboard_mapping.html from keyboard_mapping.json')


if __name__ == '__main__':
    main()
