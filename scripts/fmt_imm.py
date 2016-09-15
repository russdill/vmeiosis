#!/usr/bin/python3

import re
import math
import struct

def uwidth(v):
    return [x for x in [8, 16, 32, 64] if v < 1 << x][0] // 8

def swidth(v):
    return [x for x in [8, 16, 32, 64] if (v >= 0 and v < (1 << (x - 1))) or (v < 0 and v >= -(1 << (x - 1)))][0] // 8

def encode_int(d, radix):
    v = int(d['s'] + d['v'], radix)
    width = {'HH': 1, 'H': 2, 'S': 2, 'L': 4, 'LL': 8, '': 0}[d['w']]
    signed = (d['u'] == '' and radix in (10, 8)) or v < 0
    if width:
        pass
    elif radix in (10, 8):
        if signed:
            width = swidth(v)
        else:
            width = uwidth(v)
    else:
        bits = len(d['v']) * {2: 1, 16: 4}[radix]
        width = 1 << max(math.ceil(math.log2(bits)) - 3, 0)
    return v.to_bytes(width, byteorder='little', signed=signed)

def encode_nan(d, radix):
    fmt, width = {'F': ('f', 4), 'D': ('d', 4), '': ('f', 4)}[d.get('w', '')]
    v = float(d['s'] + 'nan')
    m = int(d['m'], 0) if d['m'] else 0
    mbits = {4: 23, 8: 52}[width]
    if m >= 1 << mbits:
        return None
    data = int.from_bytes(struct.pack(f'<{fmt}', v), byteorder='little', signed=False)
    data &= ~((1 << mbits) - 1)
    data |= m
    return data.to_bytes(width, byteorder='little', signed=False)

def encode_inf(d, radix):
    fmt = {'F': 'f', 'D': 'd', '': 'f'}[d['w']]
    v = float(d['s'] + 'inf')
    return struct.pack(f'<{fmt}', v)

def encode_float(d, radix):
    fmt = {'F': 'f', 'D': 'd', '': 'f'}[d['w']]
    v = float(d['s'] + d['v'])
    return struct.pack(f'<{fmt}', v)

def encode_hexfloat(d, radix):
    fmt = {'F': 'f', 'D': 'd', '': 'f'}[d['w']]
    v = float.fromhex(d['s'] + d['v'] + d['e'])
    return struct.pack(f'<{fmt}', v)

def encode_s(s):
    return b''.join([n.encode('latin-1' if ord(n) < 0x100 else 'utf-8') for n in s.encode('latin-1').decode('unicode-escape')])

def encode_ch(d, radix):
    ret = encode_s(d['ch'])
    return ret if len(ret) == 1 else None


str_re = [
    (r'(?P<s>[-+]?)0[xX](?P<v>[0-9A-Fa-f]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 0, 16, encode_int),
    (r'(?P<s>[-+]?)0(?P<v>[0-7]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 0, 8, encode_int),
    (r'(?P<s>[-+]?)0[bB](?P<v>[01]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 0, 2, encode_int),
    (r'(?P<s>[-+]?)(?P<v>([1-9][0-9]*|0))(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 0, 10, encode_int),

    (r'(?P<s>[-+]?)(?P<v>[0-9A-Fa-f]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 16, 16, encode_int),
    (r'(?P<s>[-+]?)(?P<v>[0-7]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 8, 8, encode_int),
    (r'(?P<s>[-+]?)(?P<v>[01]+)(?P<w>HH|H|S|L|LL|)(?P<u>U?)', 2, 2, encode_int),

    (r'(?P<s>[-+]?)(?i:NAN)(?P<m>0[0-7]+|[1-9][0-9]*|0)?(?P<w>[DF]?)', 0, None, encode_nan),
    (r'(?P<s>[-+]?)(?i:NAN)(?P<m>0[xX][0-9A-Fa-f]+)?', 0, None, encode_nan),
    (r'(?P<s>[-+]?)(?i:INF(INITY)?)(?P<w>[DF]?)', 0, None, encode_inf),
    (r'(?P<s>[-+]?)(?P<v>[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?)(?P<w>[DF]?)', 0, None, encode_float),
    (r'(?P<s>[-+]?)(?P<v>0[xX][0-9a-fA-F]+(\.[0-9A-Fa-f]+)?)((?P<e>[pP][-+]?[0-9]+)(?P<w>[DF])?)?', 0, None, encode_hexfloat),

    (r"'(?P<ch>.*)'", 0, None, encode_ch),
    (r'"(?P<str>.*)"', 0, None, lambda d, radix: encode_s(d['str'])),
    (r'^', 0, None, lambda d, radix: b''),
]

def encode_line(line, radix, terminator=r'\s*(,\s*|$)'):
    orig_line = line
    data = b''
    while line:
        for r, auto_radix, actual_radix, kind in str_re:
            if radix != 0 and radix != actual_radix:
                # User has selected radix, actual radix must match
                continue
            if radix == 0 and auto_radix != 0:
                # User has selected auto, ignore non-auto entries
                continue
            m = re.match(r'^\s*' + r + terminator, line)
            if m:
                break
        if not m:
            raise Exception(f'Invalid data: "{orig_line}"')
        line = line[len(m.group(0)):]
        d = m.groupdict(default='')
        c = kind(d, actual_radix)
        if c is None:
            raise Exception(f'Invalid data: "{orig_line}"')
        data += c
    return data

class FmtImm:
    id = 'm'
    desc = 'imm'

    def __init__(self, part):
        self.part = part

    def op_detect_str(self, s):
        try:
            data = encode_line(s, 0)
        except:
            return 0
        return 10 if len(data) > 0 else 0

    def op_output_file(self, s, segments):
        raise Exception('Immediate format not supported for output')

    def op_input_str(self, s):
        return [(0, encode_line(s, 0))]


class FmtNum:
    def __init__(self, part):
        self.part = part

    def op_detect_file(self, f):
        start, data = self.op_input_file(f)
        return 20 if len(data) > 0 else 0

    def op_output_file(self, f, segments):
        prefix, fmt = {2: ('0b', 'b'), 8: ('0', 'o'), 10: ('', 'd'), 16: ('0x', 'x')}[self.radix]
        first = True
        for seg in segments:
            for b in seg[1]:
                if not first:
                    f.write(',')
                first = False
                if self.radix == 8 and b < 8:
                    f.write(f'{b:d}')
                else:
                    f.write(f'{prefix}{b:{fmt}}')
        f.write('\n')

    def op_input_file(self, f):
        data = b''
        for line in f.readlines():
            data += encode_line(line.strip(), 0, terminator=r'\s*(,\s*#.*|,\s*|#.*|$)')
        return [(0, data)]

class FmtBin(FmtNum):
    id = 'b'
    desc = 'bin'
    radix = 2

class FmtDec(FmtNum):
    id = 'd'
    desc = 'bin'
    radix = 10

class FmtHex(FmtNum):
    id = 'h'
    desc = 'bin'
    radix = 16

class FmtOct(FmtNum):
    id = 'o'
    desc = 'bin'
    radix = 8

formats = [FmtImm, FmtBin, FmtDec, FmtHex, FmtOct]
