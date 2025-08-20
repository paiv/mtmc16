#!/usr/bin/env python
import json
import re
from pathlib import Path


def process_file(fp, so, strip=False):
    name = Path(fp.name).name
    name = re.sub(r'[^a-zA-Z0-9]', '_', name)
    name = args.prefix + name
    decl = f'static const GLchar* {name} ='
    print(decl, file=so)
    for line in fp:
        if strip:
            line = re.sub(r'(?<!\\)//.*$', '', line)
        line = line.rstrip()
        if line:
            s = json.dumps(line + '\n', ensure_ascii=False)
            print('    ' + s, file=so)
    print('    ;\n', file=so)


def main(args):
    for fp in args.file:
        process_file(fp, args.output, strip=args.strip_comments)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Embed files as C strings.')
    parser.add_argument('-c', '--strip-comments', action='store_true',
        help='remove C-style comments')
    parser.add_argument('-p', '--prefix', default='_',
        help='add prefix to variable names (default: %(default)s)')
    parser.add_argument('-o', '--output', type=argparse.FileType('w'),
        help='output file')
    parser.add_argument('file', nargs='+', type=argparse.FileType('r'),
        help='input files')
    args = parser.parse_args()
    main(args)
