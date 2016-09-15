#!/usr/bin/python3

class FmtRBin:
    id = 'r'
    desc = 'rbin'

    def __init__(self, part):
        self.part = part

    def op_detect_bin(self, f):
        while True:
            ch = f.read(1)
            if ch == b'':
                return 0
            if ord(ch) > 0x7f or ord(ch) == 0x00:
                return 1

    def op_output_bin(self, f, segments):
        for seg in segments:
            f.write(seg[1])

    def op_input_bin(self, f):
        return [(0, f.read())]

formats = [FmtRBin]
