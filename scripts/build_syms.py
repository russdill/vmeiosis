#!/usr/bin/python3

import sys

areas = {}

for line in sys.stdin:
    line = line.split()
    if len(line) < 3:
        continue
    sz = int(line.pop(1), 0x10) if len(line) > 3 else 0
    addr, _type, sym = line
    addr = int(addr, 0x10)
    areas.setdefault(_type, []).append((addr, sz, sym))

for a in areas.values():
    a.sort(key=lambda n: n[0])

print('__attribute__((naked,used)) static void __bl_addresses(void)')
print('{')
print('\tasm(')

print(r'".pushsection .bl_addresses, \"\", @note\n"')
for addr, sz, sym in areas['T']:
    if not sym.startswith('__bl_') or sym.startswith('__bl___vector_'):
        continue
    sym = sym.removeprefix('__bl_')
    print(f'".set {sym}, 0x{addr:x} + __TEXT_REGION_ORIGIN__\\n"')
    print(f'".type {sym}, @function\\n"')
    print(f'".global {sym}\\n"')
print(r'".popsection\n"')

print(r'".pushsection .bss\n"')
for addr, sz, sym in areas['B']:
    if sym.startswith('__bss_'):
        continue
    print(f'".set {sym}, 0x{addr:x}\\n"')
    print(f'".size {sym}, 0x{sz:x}\\n"')
    print(f'".zero 0x{sz:x}\\n"')
    print(f'".global {sym}\\n"')
print(r'".popsection\n"')
print('\t);')
print('}')
