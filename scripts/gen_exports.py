#!/usr/bin/env python3
# CZDS: Xash (Windows) resolves Think/Touch/Use pointers via the DLL export table
# and supports max 4096 exports. Build a .def with only the needed symbols.
# usage: gen_exports.py <mp.dll> <regamedll/dlls src root> <out.def>
import re, sys, pathlib, pefile

dll, src, out = sys.argv[1:4]
names = set()
pat = re.compile(r'Set(?:Think|Touch|Use|Blocked|MoveDone)\s*\(\s*&\s*([A-Za-z_0-9]+(?:::[A-Za-z_0-9]+)*)')
for p in pathlib.Path(src).rglob('*'):
    if p.suffix in ('.cpp', '.h'):
        for m in pat.finditer(p.read_text(errors='ignore')):
            names.add(m.group(1))
pairs = set()
for n in names:
    parts = n.split('::')
    pairs.add(('?' + parts[-1] + '@' + '@'.join(reversed(parts[:-1])) + '@@'))
pe = pefile.PE(dll)
keep = []
for s in pe.DIRECTORY_ENTRY_EXPORT.symbols:
    if not s.name: continue
    n = s.name.decode()
    if not n.startswith('?') or any(n.startswith(p) for p in pairs):
        m = re.fullmatch(r'_([A-Za-z0-9_]+)@\d+', n)  # x86 stdcall: let the linker decorate the name itself
        keep.append(m.group(1) if m else n)
if 'GiveFnptrsToDll' not in keep:
    keep.append('GiveFnptrsToDll')
print('total exports', len(pe.DIRECTORY_ENTRY_EXPORT.symbols), 'kept', len(keep), 'wanted', len(pairs))
if len(keep) > 4000:
    sys.exit('too many exports')
open(out, 'w').write('EXPORTS\n' + ''.join('\t' + n + '\n' for n in sorted(set(keep))))
