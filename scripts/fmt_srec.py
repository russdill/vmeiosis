#!/usr/bin/python3

import re

class FmtSrec:
    id = 's'
    desc = 'ihex'

    def __init__(self, part):
        self.part = part

    def op_detect_file(self, f):
        lines = 0
        for line in f.readlines():
            line = line.strip()
            if not line:
                continue
            m = re.fullmatch(r'^S([0-9])([0-9A-Fa-f]{2})(([0-9A-Fa-f]{2})+)', line)
            if not m:
                return 0
            lines += 1
        return 100 if lines > 1 else 0

    def op_output_file(self, f, segments):
        rec_count = 0
        for seg in segments:
            offset = 0
            addr = seg[0]
            data = seg[1]
            while offset < len(data):
                d = data[offset:offset + 16]
                if addr + offset < 0x10000:
                    a = (addr + offset).to_bytes(2, 'big')
                    _type = 1
                elif addr + offset < 0x1000000:
                    a = (addr + offset).to_bytes(3, 'big')
                    _type = 2
                else:
                    a = (addr + offset).to_bytes(4, 'big')
                    _type = 3
                count = len(a) + len(d) + 1
                crc = 0xff - ((count + sum(a) + sum(d)) & 0xff)
                h = bytes([count]) + a + d + bytes([crc])
                f.write(f'S{_type}' + h.hex().upper() + '\n')
                rec_count += 1
                offset += len(d)
        if rec_count < 0x10000:
            addr = rec_count.to_bytes(2, 'big')
            _type = 5
        else:
            addr = rec_count.to_bytes(3, 'big')
            _type = 6
        count = len(addr) + 1
        crc = 0xff - ((count + sum(addr)) & 0xff)
        h = bytes([count]) + addr + bytes([crc])
        f.write(f'S{_type}' + h.hex().upper() + '\n')

    def op_input_file(self, f):
        image = {}
        lineno = 0
        rec_count = 0
        for line in f.readlines():
            line = line.strip()
            lineno += 1
            if not line:
                continue
            m = re.fullmatch(r'^S([0-9])([0-9A-Fa-f]{2})(([0-9A-Fa-f]{2})+)', line)
            if m:
                _type, count, data, _ = m.groups()
                _type = int(_type)
                if _type == 4:
                    raise Exception(f'Invalid record type field in record on line {lineno}')
                count = int(count, 0x10)
                data = bytearray.fromhex(data)
                if count != len(data) or count in (0, 1, 2):
                    raise Exception(f'Invalid count field in record on line {lineno}')
                crc = data.pop()
                if crc != 0xff - ((count + sum(data)) & 0xff):
                    raise Exception(f'CRC mismatch in record on line {lineno}')
                addrlen = [2, 2, 3, 4, None, 2, 3, 4, 3, 2][_type]
                addr = int.from_bytes(data[:addrlen], 'big', signed=False)
                data = data[addrlen:]
                if _type in (1, 2, 3):
                    rec_count += 1
                    for o, d in enumerate(data):
                        image[addr + o] = d
                elif _type in (5, 6):
                    if rec_count != addr:
                        raise Exception('File contains missing records')

        segments = []
        seg_start = None
        for addr, b in image.items():
            if seg_start is None:
                seg_start = addr
                seg_data = []
            elif addr != seg_curr + 1:
                segments.append((seg_start, seg_data))
                seg_start = addr
                seg_data = []
            seg_curr = addr
            seg_data.append(b)
        if seg_start is not None:
            segments.append((seg_start, bytes(seg_data)))
        return segments

formats = [FmtSrec]
