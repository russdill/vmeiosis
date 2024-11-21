#!/usr/bin/python
#
# Copyright (C) 2016 Russ Dill <russ.dill@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import usb.core
import argparse
import struct
import time
import sys
import progress.bar
import progress.spinner
import avrdude_conf
import fmt_elf
import fmt_ihex
import fmt_imm
import fmt_rbin
import fmt_srec

avr_formats = \
    fmt_elf.formats + \
    fmt_ihex.formats + \
    fmt_imm.formats + \
    fmt_rbin.formats + \
    fmt_srec.formats

def hexdump(src, length=16, sep='.', offset=0):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or sep for x in range(256)])
    lines = []
    last_chars = None
    was_continue = False
    for c in range(0, len(src), length):
        chars = src[c: c + length]
        hex_ = ' '.join([f'{x:02x}' for x in chars])
        if len(hex_) > 24:
            hex_ = ' '.join([hex_[:24], hex_[24:]])
        printable = ''.join([str((x <= 127 and FILTER[x]) or sep) for x in chars])
        if chars == last_chars:
            if not was_continue:
                lines.append('*')
                was_continue = True
        else:
            lines.append(f'{c+offset:08x}  {hex_:{length * 3}}  |{printable:{length}}|')
            was_continue = False
        last_chars = chars
    if was_continue:
        c = len(src) - (len(src) % -length)
        lines.append(f'{c+offset:08x}')
    return '\n'.join(lines)

class Progress:
    def __init__(self, title):
        self.progress = progress.bar.Bar(title, suffix='')
    def start(self, _max):
        self.progress.max = _max
    def next(self):
        self.progress.next()
    def finish(self):
        self.progress.finish()

class ProgressNone:
    def start(self, _max):
        pass
    def next(self):
        pass
    def finish(self):
        pass

meiosis_max_major = 2
meiosis_min_major = 2
idVendor = 0x16c0
idProduct = 0x05dc
manufacturer = 'russd@asu.edu'
product = 'vme'

# Commands
meiosis_buf_write = 1
meiosis_page_erase = 3
meiosis_page_write = 5
meiosis_dev_read = 10
meiosis_exit = 128
meiosis_enter = 0

meiosis_dev_read_flash = (1 << 0)
meiosis_dev_read_fuse = (1 << 3) | (1 << 0)
meiosis_dev_read_sig = (1 << 5) | (1 << 0)
meiosis_dev_read_eeprom = (1 << 6)
meiosis_dev_read_mem = 0

class find_id:
    def __init__(self, bus, addr):
        self.bus = bus
        self.addr = addr

    def __call__(self, dev):
        if self.bus is not None and self.bus != dev.bus:
            return False
        if self.addr is not None and self.addr != dev.address:
            return False
        return True

avrdev_readers = {
    'flash': (meiosis_dev_read_flash, 0),
    'eeprom': (meiosis_dev_read_eeprom, 0),
    'fuse': (meiosis_dev_read_fuse, 0),
    'lfuse': (meiosis_dev_read_fuse, 0),
    'hfuse': (meiosis_dev_read_fuse, 3),
    'efuse': (meiosis_dev_read_fuse, 2),
    'lock': (meiosis_dev_read_fuse, 1),
    'lockbits': (meiosis_dev_read_fuse, 1),
    'signature': (meiosis_dev_read_sig, 0),
    'sram': (meiosis_dev_read_mem, 0),
    'io': (meiosis_dev_read_mem, 0),
}

class AVRDev:
    def __init__(self, usb_dev):
        self.usb = usb_dev
        self.dry = False

    def reenumerate(self, request, progress=None):
        port_numbers = self.usb.port_numbers
        self.cmd(request)
        self.usb = None
        slept = 0.0
        progress.start()
        while True:
            time.sleep(0.100)
            slept += 0.100
            if slept >= 1.500:
                self.usb = usb.core.find(port_numbers=port_numbers)
                if self.usb:
                    break
            if slept >= 5.0:
                raise Exception('Device did not return')
            progress.next()
        time.sleep(0.100)
        progress.finish()

    def cmd(self, request, value=0, index=0):
        if not self.dry or (request in meiosis_exit, meiosis_enter):
            #print(f'0x40 {request=:x} {value=:x} {index=:x}')
            self.usb.ctrl_transfer(0x40, request, value, index, None)

    def read(self, request, index=0, _len=0):
        ret = b''
        #print(f'{request=:x} {value=:x} {index=:x} {_len=:x}')
        while _len != len(ret):
            #print(f'{request=:x} {value=:x} {index+len(ret)=:x} {min(_len, 8)=:x}')
            rd = self.usb.ctrl_transfer(0xc0, request, 0, index + len(ret), min(_len - len(ret), 8))
            if len(rd) != min(_len - len(ret), 8):
                raise Exception(f'Short read on {self}')
            ret += rd
        #print('done')
        return ret

    def erase_device(self, progress=ProgressNone()):
        progress.start(self.num_user_pages)
        for page in range(self.num_user_pages, 0, -1):
            #print(f'Erase page {(page - 1) * self.page_size:x}')
            self.cmd(meiosis_page_erase, 0, (page - 1) * self.page_size)
            time.sleep(self.erase_sleep)
            progress.next()
        progress.finish()

    # Transfer the data to the microcontroller
    def write_flash(self, start, data, progress=ProgressNone(), finish=False):
        def run(just_count):
            count = 0
            empty = True
            wps = self.page_size // self.n_page_erase
            for i, w in enumerate(struct.iter_unpack('<H', data)):
                last = i * 2 + 2 == len(data) # Allow partial page write at end
                w = w[0]
                i = i * 2 + start
                if i >= self.user_size and not finish:
                    w = 0xffff
                if w != 0xffff:
                    empty = False
                    if not just_count:
                        #print(f'  {i:03x}={w:04x}')
                        self.cmd(meiosis_buf_write, w, i)
                i += 2
                if ((i % wps) == 0 or last) and not empty:
                    if not just_count:
                        page = (i - 1) & ~(wps - 1)
                        #print(f'write {page=:x}')
                        self.cmd(meiosis_page_write, 0, page)
                        time.sleep(self.write_sleep)
                        progress.next()
                    empty = True
                    count += 1
            return count
        if not finish and start + len(data) > self.user_size:
            end_start = max(0, start - self.user_size)
            data_start = end_start + self.user_size
            end_len = start + len(data) - data_start
            #print(f'{end_start=:x} {end_len=:x} {data_start=:x} {len(data)=:x}')
            self.end_data[end_start:end_start + end_len] = data[data_start:]
        progress.start(run(True))
        run(False)
        progress.finish()

    def write_flash_end(self):
        #print(f'{self.user_size=:x} {len(self.end_data)=:x}')
        self.write_flash(self.user_size, self.end_data, finish=True)
        self.end_data = bytearray(b'\xff' * (self.bootloader_start - self.user_size))

    def read_region(self, region_name, start=0, length=-1, chunk_sz=64, progress=ProgressNone()):
        reader_request, reader_offset = avrdev_readers[region_name]
        reader_offset += start
        region_info = self.part_info['memory'][region_name]
        if region_name == 'signature':
            if length == -1:
                length = 3
            if reader_offset + length > 3:
                raise Exception('Read too large')
            # Always read whole block for simplicity
            read_sz = 5
            read_offset = int(region_info.get('offset', 0))
        else:
            region_sz = int(region_info.get('size', 0))
            if length == -1:
                read_sz = max(region_sz - start, 0)
                length = read_sz
            else:
                read_sz = length
                if start + read_sz > region_sz:
                    raise Exception('Read too large')
            read_offset = reader_offset + int(region_info.get('offset', 0))
        progress.start((length + chunk_sz - 1) // chunk_sz)
        data = b''
        # Read one chunk at a time for a decent progress update rate
        #print(f'{read_offset=:x} {read_sz=:x}')
        for i in range(read_offset, read_offset + read_sz, chunk_sz):
            data += self.read(reader_request, i, min(read_offset + read_sz - i, chunk_sz))
            progress.next()
        progress.finish()
        if region_name == 'signature':
            data = data[::-2]
            data = data[start:start + length]
        return data

    def probe(self, dry_run, db, signatures):
        self.dry = dry_run
        major = self.usb.bcdDevice >> 8
        if major > meiosis_max_major or major < meiosis_min_major:
            raise Exception('Unsupported version')

        info = self.read(meiosis_dev_read_sig, 0, 5)
        self.signature = ''.join([f'{n:02x}' for n in info[::2]])
        self.part_name = signatures[self.signature]
        self.part_info = db['part'][self.part_name]
        self.part_desc = self.part_info.get('desc', self.part_name)
        flash_info = self.part_info['memory']['flash']
        self.flash_size = int(flash_info['size'], 0)
        self.n_page_erase = int(self.part_info.get('n_page_erase', '1'))
        self.num_pages = int(flash_info['num_pages'], 0)
        self.page_size = self.n_page_erase * self.flash_size // self.num_pages
        self.write_sleep = int(flash_info["max_write_delay"]) / 1000000.0
        self.erase_sleep = int(self.part_info["chip_erase_delay"]) * self.n_page_erase / 1000000.0

        info = self.read(meiosis_dev_read_flash, self.flash_size - 4, 4)
        self.cfg_word_0, self.cfg_word_1 = struct.unpack('<HH', info)
        self.num_bl_pages = self.cfg_word_0 & 0xff
        self.cfg_word_0 &= ~0xff
        self.vector = (self.cfg_word_0 >> 8) & 0x1f

        self.num_user_pages = self.num_pages - self.num_bl_pages
        self.bootloader_start = self.num_user_pages * self.page_size
        end_size = 4
        self.user_size = self.bootloader_start - end_size
        self.end_data = bytearray(end_size * b'\xff')

    def __str__(self):
        major = self.usb.bcdDevice >> 8
        minor = self.usb.bcdDevice & 0xff
        s = f'Bus {self.usb.bus:03d} Device {self.usb.address:03d}: '
        s += f'ID {self.usb.idVendor:04x}:{self.usb.idProduct:04x} '
        s += f'{self.usb.manufacturer}/{self.usb.product} v{major}.{minor}'
        return s

def find_dev(bus, address):
    devs = []
    for dev in usb.core.find(find_all=True,
                idVendor=options.id_vendor, idProduct=options.id_product,
                manufacturer=options.manufacturer, product=options.product,
                custom_match=find_id(bus, address)):
        devs.append(AVRDev(dev))
    return devs

def rjmp_to_addr(data, base):
    opcode, = struct.unpack('<H', data[base:base + 2])

    if (opcode & 0xf000) != 0xc000:
        return None

    offset = ((opcode & 0xfff) + 1) * 2

    return (offset + base) & 0x1fff

def patch_rjmp(data, dest, base):
    rbase = base + 2
    if dest + 4096 < rbase:
        dest += 8192
    if dest - 4094 > rbase:
        rbase += 8192
    if dest + 4096 < rbase or dest - 4094 > rbase:
        raise Exception("rjmp out of range")

    offset = (dest - rbase) // 2
    data[base:base + 2] = struct.pack('<H', 0xc000 | (offset & 0xfff))

def patch_jmp(data, dest, base):
    data[base:base + 4] = struct.pack('<HH', 0x940c, dest // 2)

def patch_reti(data, base):
    data[base:base + 2] = struct.pack('<H', 0x9518)

def patch_firmware(dev, data, flash_range, patch_irq=True):
    # Verify user reset handler
    data = bytearray(data)
    if 0 in flash_range or 1 in flash_range:
        # Find the current user reset vector
        user_reset = rjmp_to_addr(data, 0)
        if user_reset is None:
            raise Exception('Vector table reset does not contain an rjmp')

        # Patch in rjmp to bootloader.
        patch_rjmp(data, dev.bootloader_start + 0, 0)

        # Move user reset vector to end of last page. The reset vector
        # is always the first vector in the tinyvectortable.
        patch_rjmp(data, user_reset, dev.bootloader_start - 4)

    # Check if the user has a handler for the vector v-usb is using
    vector_addr = dev.vector * 2
    if patch_irq and dev.vector and (vector_addr in flash_range or vector_addr + 1 in flash_range):
        user_vector = rjmp_to_addr(data, vector_addr)
        if user_vector:
            if flash_start >= user_vector + 2 or flash_end < user_vector:
                raise Exception('User vector target outside given memory area')
            next_vector = rjmp_to_addr(data, user_vector)
            if next_vector == 0 or next_vector == user_vector:
                # Jumps to reset vector of self (bad interrupt)
                user_vector = 0
        # Patch in rjmp to chained interrupt handler
        patch_rjmp(data, dev.flash_size - 10, vector_addr)

        # Allow chaining to a user interrupt handler.
        if user_vector:
            patch_rjmp(data, user_vector, dev.bootloader_start - 2)
        else:
            # No handler, just reti
            patch_reti(data, dev.bootloader_start - 2)
    else:
        user_vector = None
    return data

def unpatch_firmware(dev, data):
    # Verify user reset handler
    data = bytearray(data)
    if len(data) != dev.bootloader_start:
        raise Exception

    # Find the current user reset vector
    user_reset = rjmp_to_addr(data, dev.user_size)

    if user_reset is not None:
        patch_rjmp(data, user_reset, 0)
    if dev.vector:
        user_vector = rjmp_to_addr(data, dev.user_size + 2)
        if user_vector is not None:
            patch_rjmp(data, user_vector, dev.vector * 2)

    data[dev.user_size:] = 4 * b'\xff'
    return data

def parse_op(s):
    if ':' not in s:
        return ['flash'], 'w', s, 'a'
    tokens = s.split(':')
    if len(tokens) < 3:
        raise Exception('Invalid option format')
    fmt = tokens.pop() if len(tokens) > 3 and len(tokens[-1]) == 1 else 'a'
    mem = tokens[0].split(',')
    op = tokens[1]
    if op not in 'vrw':
        raise Exception(f'Unknown operation "{op}"')
    return mem, op, ':'.join(tokens[2:]), fmt

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--index', type=int, default=0, help='Index of device')
parser.add_argument('-b', '--bus', type=int, help='USB bus index')
parser.add_argument('-a', '--address', type=int, help='USB device address')
parser.add_argument('-M', '--manufacturer', help='USB device manufacturer name', default=manufacturer)
parser.add_argument('-N', '--product', help='USB device product name', default=product)
parser.add_argument('-V', '--id-vendor', help='USB device vendor ID', type=lambda x: int(x, 16), default=idVendor)
parser.add_argument('-P', '--id-product', help='USB device product ID', type=lambda x: int(x, 16), default=idProduct)
parser.add_argument('-l', '--list', action='store_true', help='List devices')
parser.add_argument('-E', '--enter', action='store_true', help='Enter bootloader')
parser.add_argument('-r', '--run', action='store_true', help='Exit bootloader')
parser.add_argument('-C', '--config-file', action='append', help='Specify location of configuration file')
parser.add_argument('-e', '--erase', action='store_true', help='Erase flash')
parser.add_argument('-U', '--mem-op', action='append', type=parse_op, help='Memory operation specification')
parser.add_argument('-n', '--dry-run', action='store_true', help='Do not write anything to the device')
parser.add_argument('-R', '--raw', action='store_true', help='Program non-vmeiosis user program (do not patch interrupt vector)')
options = parser.parse_args()

base_cf = None
ext_cf = []
for cf in options.config_file if options.config_file else []:
    if cf[0] == '+':
        ext_cf.append(cf[1:])
    elif base_cf is None:
        base_cf = cf
    else:
        raise Exception('More than one config-file specified on command-line')
if base_cf is None:
    base_cf = '/etc/avrdude.conf'

with open(base_cf, 'r') as f:
    db = avrdude_conf.parse(f)
for cf in ext_cf:
    avrdude_conf.parse(cf, db)
db_signatures = avrdude_conf.signatures(db)

devs = find_dev(options.bus, options.address)
if not devs:
    print('No devices found')
    quit()

if options.list:
    for dev in devs:
        print(dev)
    quit()

dev = devs[options.index]
if options.enter:
    print(dev)
    dev.reenumerate(meiosis_enter, progress.spinner.Spinner('  Entering bootloader mode '))
dev.probe(options.dry_run, db, db_signatures)

avr_formats = {fmt.id: fmt(dev.part_info) for fmt in avr_formats}

print(dev)
print(f'  User size {dev.user_size}')
print(f'  Page size {dev.page_size}')
print(f'  Write/erase sleep {dev.write_sleep * 1000.0:.1f}ms/{dev.erase_sleep * 1000.0:.1f}ms')
print(f'  Device signature 0x{dev.signature}, part {dev.part_desc}')

avr_region_to_file_region = {
    'eeprom': ('EEPROM', 0),
    'flash': ('flash', 0),
    'fuse': ('fuse', 0),
    'lfuse': ('fuse', 0),
    'hfuse': ('fuse', 1),
    'efuse': ('fuse', 2),
    'lock': ('lock', 0),
    'lockbits': ('lock', 0),
    'signature': ('sigrow', 0),
    'io': None, # read_mem with given offset
    'sram': None, # read_mem with given offset
}

file_regions = [
    (0x000000, 0x800000, "flash"),
    (0x800000, 0x010000, "data"),
    (0x810000, 0x010000, "EEPROM"),
    (0x820000, 0x010000, "fuse"),
    (0x830000, 0x010000, "lock"),
    (0x840000, 0x010000, "sigrow"),
    (0x850000, 0x010000, "userrow"),
    (0x860000, 0x010000, "bootrow"),
]

def file_region_by_addr(addr):
    for idx, region in enumerate(file_regions):
        if addr >= region[0] and addr < region[0] + region[1]:
            return idx
    return None

def file_region_by_name(name):
    for idx, region in enumerate(file_regions):
        if name == region[2]:
            return idx
    return None

def op_detect(fmt, fn):
    try:
        if hasattr(fmt, 'op_detect_bin'):
            if fn == '-':
                return fmt.op_detect_bin(sys.stdin.buffer)
            else:
                with open(fn, 'rb') as f:
                    return fmt.op_detect_bin(f)
        elif hasattr(fmt, 'op_detect_file'):
            if fn == '-':
                return fmt.op_detect_file(sys.stdin)
            else:
                with open(fn, 'r') as f:
                    return fmt.op_detect_file(f)
        elif hasattr(fmt, 'op_detect_str'):
            return fmt.op_detect_str(fn)
        else:
            raise Exception
    except:
        return 0

def op_input(fmt, fn):
    if hasattr(fmt, 'op_input_bin'):
        if fn == '-':
            return fmt.op_input_bin(sys.stdin.buffer)
        else:
            with open(fn, 'rb') as f:
                return fmt.op_input_bin(f)
    elif hasattr(fmt, 'op_input_file'):
        if fn == '-':
            return fmt.op_input_file(sys.stdin)
        else:
            with open(fn, 'r') as f:
                return fmt.op_input_file(f)
    elif hasattr(fmt, 'op_input_str'):
        return fmt.op_input_str(fn)
    else:
        raise Exception

def op_output(fmt, fn, segments):
    if hasattr(fmt, 'op_output_bin'):
        if fn == '-':
            fmt.op_output_bin(sys.stdout.buffer, segments)
        else:
            with open(fn, 'wb') as f:
                fmt.op_output_bin(f, segments)
    elif hasattr(fmt, 'op_output_file'):
        if fn == '-':
            fmt.op_output_file(sys.stdout, segments)
        else:
            with open(fn, 'w') as f:
                fmt.op_output_file(f, segments)
    else:
        raise Exception

if options.erase:
    dev.erase_device(Progress('  Erasing '))
    erased = True
else:
    erased = False

write_end = False
verify_end = False
vectors_programmed = False

mem_op = []
flash_written = False
eeprom_written = False
eeprom_writer = None
for avr_mems, op, fn, fmt_spec in options.mem_op or []:
    # Check that we support the format requested
    fmt = avr_formats.get(fmt_spec, None)
    if fmt is None and fmt_spec == 'a' and op in 'wv':
        score, fmt = sorted([(op_detect(f, fn), f) for f in avr_formats.values()], key=lambda x: x[0])[-1]
        if not score:
            raise Exception(f'Could not auto-detect format for {fn}')
    elif fmt is None:
        raise Exception(f'Unknown format for {fn}, "{fmt_spec}"')

    # Figure out the actual list of regions
    am = []
    for avr_mem in avr_mems:
        remove = avr_mem[0] in '\\-'
        if remove:
            avr_mem = avr_mem[1:]
        if avr_mem.lower() == 'all' or avr_mem == 'etc':
            for name, info in dev.part_info['memory'].items():
                if name in ('io', 'sram') or name not in avr_region_to_file_region:
                    continue
                if avr_mem == 'ALL' and (name == 'signature' or 'fuse' in name):
                    continue
                if remove:
                    if name in am:
                        am.remove(name)
                elif name not in am:
                    am.append(name)
        elif avr_mem == 'none':
            pass
        elif avr_mem not in avr_region_to_file_region:
            raise Exception(f'Unsupported mem type {avr_mem}')
        elif remove:
            if avr_mem in am:
                am.remove(avr_mem)
        elif avr_mem not in am:
            am.append(avr_mem)
    avr_mems = am

    # Read in any necessary data from strings/files
    host_file_segments = []
    host_avr_segments = {}
    end_address = 0
    if op in 'wv':
        # Split segments on region boundaries
        for start, data in op_input(fmt, fn):
            end_address = max(end_address, start + len(data))
            idx = file_region_by_addr(start)
            rstart, rlen, rname = file_regions[idx]
            while start + len(data) > rstart + rlen:
                split_len = rstart + rlen - start
                host_file_segments.append((start, data[:split_len]))
                data = data[split_len:]
                idx += 1
                rstart, rlen, rname = file_regions[idx]
                start = rstart
            host_file_segments.append((start, data))

        # Check for the EEPROM writer binary and user signature
        for start, data in host_file_segments:
            idx = file_region_by_addr(start)
            rstart, rlen, rname = file_regions[idx]
            if rname == 'userrow':
                if start == rstart and len(data) > dev.page_size + 4:
                    cfg_word_0, cfg_word_1 = struct.unpack_from('<HH', data, dev.page_size)
                    if cfg_word_0 != dev.cfg_word_0 or cfg_word_1 != dev.cfg_word_1:
                        raise Exception(f'User signature in {fn} does not match bootloader')
                    eeprom_writer = data
        # Merge data section onto flash section
        flash_end = 0
        data_segments = []
        # Find the end of the flash segments
        for start, data in host_file_segments:
            idx = file_region_by_addr(start)
            rstart, rlen, rname = file_regions[idx]
            if rname == 'flash':
                flash_end = max(flash_end, start + len(data))

        merged = []
        for start, data in host_file_segments:
            idx = file_region_by_addr(start)
            rstart, rlen, rname = file_regions[idx]
            if rname == 'data':
                # Move the data segments to the end of the flash segments
                merged.append((start - rstart + flash_end, data))
            else:
                merged.append((start, data))
        host_file_segments = merged

        # Figure out the largest address
        for start, data in host_file_segments:
            end_address = max(end_address, start + len(data))

        # Modify the file segments to be avr memory segments
        if end_address <= file_regions[1][0]:
            # No region specific data
            if len(avr_mems) > 1:
                # Multiple regions listed, only accept 1
                avr_mems = [n for n in avr_mems if n == 'flash']
            if len(avr_mems) > 0:
                host_avr_segments[avr_mems[0]] = host_file_segments
        else:
            # Region specific data, filter by region type
            for avr_mem in avr_mems:
                file_region_name, foffset = avr_region_to_file_region.get(avr_mem, None)
                if not file_region_name:
                    continue
                avr_segments = []
                for start, data in host_file_segments:
                    idx = file_region_by_addr(start)
                    rstart, rlen, frname = file_regions[idx]
                    if file_region_name == frname:
                        avr_segments.append((start - rstart + foffset, data))
                if avr_segments:
                    host_avr_segments[avr_mem] = avr_segments

        if 'eeprom' in host_avr_segments:
            if flash_written:
                raise Exception('EEPROM must be written before flash')
            eeprom_written = True

        if 'flash' in host_avr_segments:
            flash_written = True

        for rname, segments in host_avr_segments.items():
            start, data = segments[0]
            if rname == 'signature':
                if start == 0 and len(data) >= len(dev.signature):
                    if data[:len(dev.signature)] != dev.signature:
                        raise Exception(f'Device signature in {fn} does not match bootloader')


    mem_op.append((avr_mems, op, fn, fmt, host_avr_segments))

if eeprom_written and not flash_written and not options.erase:
    raise Exception('Unable to write EEPROM without erasing device')

if eeprom_written and not eeprom_writer:
    raise Exception("Unable to write EEPROM without EEPROM writer code")

for avr_mems, op, fn, fmt, host_avr_segments in mem_op:
    if op in 'wv':
        eeprom_image = b''
        eeprom_offset = 0
        for start, data in host_avr_segments.get('eeprom', []):
            while start - eeprom_offset > 254:
                eeprom_image += struct.pack('<BB', 254, 0)
                eeprom_offset += 254
            while len(data):
                eeprom_image += struct.pack('<BB', start - eeprom_offset, min(len(data), 256) & 0xff)
                eeprom_image += data[:256]
                eeprom_offset += min(len(data), 256)
                start += min(len(data), 256)
                data = data[256:]
        if eeprom_image:
            eeprom_image = eeprom_writer + eeprom_image
            if not erased:
                dev.erase_device(Progress('  Erasing '))
                erased = True
            flash_mem = eeprom_image + b'\xff' * (dev.bootloader_start - len(eeprom_image))
            patched_flash_mem = patch_firmware(dev, flash_mem, range(0, len(eeprom_image)))
            dev.write_flash(0, patched_flash_mem, Progress('  Flashing EEPROM writer'))
            dev.write_flash_end()
            erased = False
            dev.reenumerate(meiosis_exit, progress.spinner.Spinner('  EEPROM writer running '))
            dev.probe(options.dry_run, db, db_signatures)

        if op == 'v':
            for start, data in host_avr_segments.get('eeprom', []):
                readback = dev.read_region('eeprom', start, len(data), progress=Progress('  Verifying EEPROM '))
                if readback != data:
                    raise Exception('Readback mismatch when verifying EEPROM')

        flash_mem = bytearray(b'\xff' * dev.bootloader_start)
        flash_start = 0
        flash_end = 0
        for start, data in host_avr_segments.get('flash', []):
            # Trim empty flash
            data = data.rstrip(b'\xff')
            if len(data) % 2:
                data += b'\xff'
            ls_len = len(data.lstrip(b'\xff'))
            if ls_len % 2:
                ls_len += 1
            data = data[len(data) - ls_len:]
            start += len(data) - ls_len
            flash_start = min(flash_start, start)
            flash_end = max(flash_end, start + len(data))
            flash_mem[start:start + len(data)] = data

        if flash_end:
            if flash_end > dev.user_size:
                raise Exception('Image does not fit within user flash area')
            if flash_start != 0 and not vectors_programmed:
                raise Exception('Vector page of flash *must* be programmed first')
            if flash_start == 0 and vectors_programmed:
                raise Exception('Vector page of flash cannot be programmed twice')
            vectors_programmed = True

            patched_flash_mem = patch_firmware(dev, flash_mem, range(flash_start, flash_end), patch_irq=not options.raw)
            if not erased:
                dev.erase_device(Progress('  Erasing  '))
                erased = True
            dev.write_flash(flash_start, patched_flash_mem[flash_start:], progress=Progress('  Flashing '))
            write_end = True
            verify_end = verify_end or op == 'v'

            if op == 'v':
                readback = dev.read_region('flash', flash_start, flash_end - flash_start, progress=Progress('  Verifying '))
                if readback != patched_flash_mem[flash_start:flash_end]:
                    raise Exception('Readback mismatch when verifying flash')

        # Can "verify" only
        for avr_mem in [n for n in avr_region_to_file_region.keys() if n not in ('eeprom', 'flash', 'io', 'sram')]:
            for start, data in host_avr_segments.get(avr_mem, []):
                readback = dev.read_region(avr_mem, start, len(data))
                if data != readback:
                    raise Exception(f'Cannot write to region {avr_mem} and existing data does not match')


    else: # op == 'r'
        file_segments = []
        for avr_mem in avr_mems:
            if avr_mem == 'flash':
                sz = dev.bootloader_start
            else:
                sz = -1
            data = dev.read_region(avr_mem, length=sz, progress=Progress(f'  Reading {avr_mem}'))
            if avr_mem == 'flash':
                data = unpatch_firmware(dev, data)
                data = data.rstrip(b'\xff')
            file_region_name, file_offset = avr_region_to_file_region[avr_mem]
            idx = file_region_by_name(file_region_name)
            fstart = file_regions[idx][0]
            file_segments.append((fstart + file_offset, bytearray(data)))
        ends = []
        starts = []
        for start, data in file_segments:
            starts.append(start)
            ends.append((start + len(data), data))
        # Merge segments where possible
        while True:
            remove = None
            for a_idx, a in enumerate(file_segments):
                a_start, a_data = a
                for b_idx, b in enumerate(file_segments):
                    b_start, b_data = b
                    if a_idx == b_idx:
                        continue
                    if b_start < a_start + len(a_data) and b_start >= a_start:
                        a_data[a_start - b_start:] = b_data[:a_start + len(a_data) - b_start]
                        b_data = b_data[a_start + len(a_data) - b_start:]
                        b_start = a_start + len(a_data)
                    if b_start == a_start + len(a_data) and len(b_data):
                        a_data.extend(b_data)
                        remove = b_idx
                        break
                if remove:
                    break
            if remove:
                file_segments.pop(remove)
            else:
                break

        op_output(fmt, fn, file_segments)

if write_end:
    end_data = dev.end_data
    dev.write_flash_end()
if verify_end:
    end = dev.read_region('flash', dev.user_size, dev.bootloader_start - dev.user_size)
    if end != end_data:
        raise Exception('Verify mismatch when writing end page')

if options.run:
    print('  Running app ...', end=' ')
    dev.cmd(meiosis_exit)#, 0x8080, 0x8080)
    print('Done')
