#!/usr/bin/python3

import intelhex

class FmtIHex:
    id = 'i'
    desc = 'ihex'

    def __init__(self, part):
        self.part = part

    def op_detect_file(self, f):
        data = intelhex.IntelHex(f)
        return 100 if len(data.segments()) > 0 else 0

    def op_output_file(self, f, segments):
        hf = intelhex.IntelHex()
        for seg in segments:
            hf.puts(seg[0], bytes(seg[1]))
        hf.write_hex_file(f)

    def op_input_file(self, f):
        hf = intelhex.IntelHex(f)
        return [(seg[0], hf.gets(seg[0], seg[1] - seg[0])) for seg in hf.segments()]

formats = [FmtIHex]
