#!/usr/bin/python3

import elftools.elf.elffile
import elftools.elf.constants
import elftools.elf.structs
import elftools.construct.core

mcus = {
    'avr1': 1,
    'avr2': 2,
    'avr25': 25,
    'avr3': 3,
    'avr31': 31,
    'avr35': 35,
    'avr4': 4,
    'avr5': 5,
    'avr51': 51,
    'avr6': 6,
    'avrtiny': 100,
    'avrxmega1': 101,
    'avrxmega2': 102,
    'avrxmega3': 103,
    'avrxmega4': 104,
    'avrxmega5': 105,
    'avrxmega6': 106,
    'avrxmega7': 107,
}

mcuids = {
    'avr1': [
        '1200', 't11', 't12', 't15', 't28',
    ],
    'avr25': [
        'at86rf401', 'ata5272', 'ata6616c', 't13', 't13a', 't2313',
        't2313a', 't24', 't24a', 't25', 't261', 't261a',
        't4313', 't43u', 't44', 't441', 't44a', 't45',
        't461', 't461a', 't48', 't828', 't84', 't841',
        't84a', 't85', 't861', 't861a', 't87', 't88',
    ],
    'avr2': [
        'c8534', '2313', '2323', '2333', '2343', '4414',
        '4433', '4434', '8515', '8535', 't22', 't26',
    ],
    'avr31': [
        'at43usb320', 'm103',
    ],
    'avr35': [
        'usb162', 'usb82', 'ata5505', 'ata6617c', 'ata664251', 'm16u2',
        'm32u2', 'm8u2', 't1634', 't167',
    ],
    'avr3': [
        'at43usb355', 'at76c711',
    ],
    'avr4': [
        'pwm1', 'pwm2', 'pwm2b', 'pwm3', 'pwm3b', 'pwm81',
        'ata6285', 'ata6286', 'ata6289', 'ata6612c', 'm48', 'm48a',
        'm48p', 'm48pa', 'm48pb', 'm8', 'm8515', 'm8535',
        'm88', 'm88a', 'm88p', 'm88pa', 'm88pb', 'm8a',
        'm8hva',
    ],
    'avr51': [
        'c128', 'usb1286', 'usb1287', 'm128', 'm1280', 'm1281',
        'm1284', 'm1284p', 'm1284rfr2', 'm128a', 'm128rfa1', 'm128rfr2',
    ],
    'avr5': [
        'c32', 'c64', 'pwm161', 'pwm216', 'pwm316', 'cr100',
        'usb646', 'usb647', 'at94k', 'ata5702m322', 'ata5782', 'ata5790',
        'ata5790n', 'ata5791', 'ata5795', 'ata5831', 'ata6613c', 'ata6614q',
        'ata8210', 'ata8510', 'm16', 'm161', 'm162', 'm163',
        'm164a', 'm164p', 'm164pa', 'm165', 'm165a', 'm165p',
        'm165pa', 'm168', 'm168a', 'm168p', 'm168pa', 'm168pb',
        'm169', 'm169a', 'm169p', 'm169pa', 'm16a', 'm16hva',
        'm16hva2', 'm16hvb', 'm16hvbrevb', 'm16m1', 'm16u4', 'm32',
        'm323', 'm324a', 'm324p', 'm324pa', 'm325', 'm3250',
        'm3250a', 'm3250p', 'm3250pa', 'm325a', 'm325p', 'm325pa',
        'm328', 'm328p', 'm328pb', 'm329', 'm3290', 'm3290a',
        'm3290p', 'm3290pa', 'm329a', 'm329p', 'm329pa', 'm32a',
        'm32c1', 'm32hvb', 'm32hvbrevb', 'm32m1', 'm32u4', 'm32u6',
        'm406', 'm64', 'm640', 'm644', 'm644a', 'm644p', 'm644pa',
        'm644rfr2', 'm645', 'm6450', 'm6450a', 'm6450p', 'm645a', 'm645p',
        'm649', 'm6490', 'm6490a', 'm6490p', 'm649a', 'm649p',
        'm64a', 'm64c1', 'm64hve', 'm64hve2', 'm64m1', 'm64rfr2',
        'm3000',
    ],
    'avr6': [
        'm2560', 'm2561', 'm2564rfr2', 'm256rfr2',
    ],
    'avrtiny': [
        't10', 't20', 't4', 't40', 't5', 't9',
    ],
    'avrxmega2': [
        'x16a4', 'x16a4u', 'x16c4', 'x16d4', 'x16e5', 'x32a4',
        'x32a4u', 'x32c3', 'x32c4', 'x32d3', 'x32d4', 'x32e5',
        'x8e5',
    ],
    'avrxmega3': [
        't1614', 't1616', 't1617', 't212', 't214', 't3216',
        
        't3217', 't412', 't414', 't416', 't417', 't814',
        't816', 't817',
    ],
    'avrxmega4': [
        'x64a3', 'x64a3u', 'x64a4u', 'x64b1', 'x64b3', 'x64c3',
        'x64d3', 'x64d4',
    ],
    'avrxmega5': [
        'x64a1', 'x64a1u',
    ],
    'avrxmega6': [
        'x128a3', 'x128a3u', 'x128b1', 'x128b3', 'x128c3', 'x128d3',
        'x128d4', 'x192a3', 'x192a3u', 'x192c3', 'x192d3', 'x256a3',
        'x256a3b', 'x256a3bu', 'x256a3u', 'x256c3', 'x256d3', 'x384c3',
        'x384d3',
    ],
    'avrxmega7': [
        'x128a1', 'x128a1u', 'x128a4u',
    ],
}

mem_segments = [
    (0x000000, 0x800000, "flash"),
    (0x800000, 0x010000, "data"),
    (0x810000, 0x010000, "EEPROM"),
    (0x820000, 0x010000, "fuses"),
    (0x830000, 0x010000, "lock"),
    (0x840000, 0x010000, "sigrow"),
    (0x850000, 0x010000, "userrow"),
    (0x860000, 0x010000, "bootrow"),
]

class FmtElf:
    id = 'e'
    desc = 'elf'

    def __init__(self, part):
        self.part = part
        _id = part['id']
        ids = [mcuid for mcuid, parts in mcuids.items() if _id in parts]
        if not ids:
            raise Exception(f'Unknown mcuid for {part.get("desc", _id)}')
        self.mcuid = mcus[ids[0]]         

    def op_detect_bin(self, f):
        elftools.elf.elffile.ELFFile(f)
        return 100

    def op_output_bin(self, f, segments):
        seg_areas = []
        for seg in segments:
            seg_area = None
            for idx, mem in enumerate(mem_segments):
                if seg[0] < mem[0] + mem[1]:
                    seg_area = mem[2]
                    break
            seg_areas.append(seg_area)

        structs = elftools.elf.structs.ELFStructs()
        structs.e_ident_osabi = 'ELFOSABI_SYSV'
        structs.e_machine = 'EM_AVR'
        structs.e_type = 'ET_EXEC'
        structs.create_basic_structs()
        structs.create_advanced_structs()

        file_offset = 0
        elements = []
        ehdr = elftools.construct.core.Container(
            e_ident=elftools.construct.core.Container(
                EI_MAG=[0x7f, 0x45, 0x4c, 0x46],
                EI_CLASS='ELFCLASS32',
                EI_DATA='ELFDATA2LSB',
                EI_VERSION=1,
                EI_OSABI=0,
                EI_ABIVERSION=0,
            ),
            e_type='ET_EXEC',
            e_machine='EM_AVR',
            e_version=1,
            e_entry=0,
            e_shoff=0,
            e_phoff=0,
            e_flags=self.mcuid,
            e_ehsize=structs.Elf_Ehdr.sizeof(),
            e_phentsize=structs.Elf_Phdr.sizeof(),
            e_phnum=0,
            e_shentsize=structs.Elf_Shdr.sizeof(),
            e_shnum=0,
            e_shstrndx=0,
        )
        elements.append((structs.Elf_Ehdr.build_stream, ehdr))

        file_offset += structs.Elf_Ehdr.sizeof()

        ehdr.e_phoff = file_offset
        phdrs = []
        for idx, seg in enumerate(segments):
            flags, align = {
                'flash': (elftools.elf.constants.P_FLAGS.PF_R | elftools.elf.constants.P_FLAGS.PF_X, 2),
                'data': (elftools.elf.constants.P_FLAGS.PF_R | elftools.elf.constants.P_FLAGS.PF_W, 1),
                'EEPROM': (elftools.elf.constants.P_FLAGS.PF_R, 1),
                'fuses': (elftools.elf.constants.P_FLAGS.PF_R, 1),
                'lock': (elftools.elf.constants.P_FLAGS.PF_R, 1),
                'sigrow': (elftools.elf.constants.P_FLAGS.PF_R, 1),
                'userrow': (elftools.elf.constants.P_FLAGS.PF_R, 1),
                'bootrow': (elftools.elf.constants.P_FLAGS.PF_R, 2),
                None: (elftools.elf.constants.P_FLAGS.PF_R, 1),
            }[seg_areas[idx]]
            if align == 2 and len(seg[1]) % 2:
                align = 1
                flags &= ~elftools.elf.constants.P_FLAGS.PF_X
            phdr = elftools.construct.core.Container(
                p_type='PT_LOAD',
                p_offset=0,
                p_vaddr=seg[0],
                p_paddr=seg[0],
                p_filesz=len(seg[1]),
                p_memsz=len(seg[1]),
                p_flags=flags,
                p_align=align
            )
            file_offset += structs.Elf_Phdr.sizeof()
            phdrs.append(phdr)
            elements.append((structs.Elf_Phdr.build_stream, phdr))
        ehdr.e_phnum = len(phdrs)

        ehdr.e_shoff = file_offset

        shdrs = []
        shdr = elftools.construct.core.Container(
            sh_name=0,
            sh_type='SHT_NULL',
            sh_flags=0,
            sh_addr=0,
            sh_offset=0,
            sh_size=0,
            sh_link=0,
            sh_info=0,
            sh_addralign=0,
            sh_entsize=0,
        )
        file_offset += structs.Elf_Shdr.sizeof()
        shdrs.append((b'', shdr))
        elements.append((structs.Elf_Shdr.build_stream, shdr))

        for idx, seg in enumerate(segments):
            name, flags, align = {
                'flash': (b'.text', elftools.elf.constants.SH_FLAGS.SHF_ALLOC | elftools.elf.constants.SH_FLAGS.SHF_EXECINSTR, 2),
                'data': (b'.data', elftools.elf.constants.SH_FLAGS.SHF_ALLOC | elftools.elf.constants.SH_FLAGS.SHF_WRITE, 1),
                'EEPROM': (b'.eeprom', elftools.elf.constants.SH_FLAGS.SHF_ALLOC, 1),
                'fuses': (b'.fuse', elftools.elf.constants.SH_FLAGS.SHF_ALLOC, 1),
                'lock': (b'.lock', elftools.elf.constants.SH_FLAGS.SHF_ALLOC, 1),
                'sigrow': (b'.signature', elftools.elf.constants.SH_FLAGS.SHF_ALLOC, 1),
                'userrow': (b'.user_signatures', elftools.elf.constants.SH_FLAGS.SHF_ALLOC, 1),
                'bootrow': (b'.boot', elftools.elf.constants.SH_FLAGS.SHF_ALLOC | elftools.elf.constants.SH_FLAGS.SHF_EXECINSTR, 2),
            }[seg_areas[idx]]
            if align == 2 and len(seg[1]) % 2:
                align = 1
                flags &= ~elftools.elf.constants.SH_FLAGS.SHF_EXECINSTR
            shdr = elftools.construct.core.Container(
                sh_name=0,
                sh_type='SHT_PROGBITS',
                sh_flags=flags,
                sh_addr=seg[0],
                sh_offset=0,
                sh_size=len(seg[1]),
                sh_link=0,
                sh_info=0,
                sh_addralign=align,
                sh_entsize=0,
            )
            file_offset += structs.Elf_Shdr.sizeof()
            shdrs.append((name, shdr))
            elements.append((structs.Elf_Shdr.build_stream, shdr))

        shdr = elftools.construct.core.Container(
            sh_name=0,
            sh_type='SHT_STRTAB',
            sh_flags=elftools.elf.constants.SH_FLAGS.SHF_ALLOC,
            sh_addr=0,
            sh_offset=0,
            sh_size=0,
            sh_link=0,
            sh_info=0,
            sh_addralign=1,
            sh_entsize=0,
        )
        file_offset += structs.Elf_Shdr.sizeof()
        shdrs.append((b'.shstrtab', shdr))
        elements.append((structs.Elf_Shdr.build_stream, shdr))
        ehdr.e_shstrndx = len(shdrs) - 1
        ehdr.e_shnum = len(shdrs)

        writer = lambda data, f: f.write(data)
        for i, seg in enumerate(segments):
            phdrs[i].p_offset = file_offset
            shdrs[i + 1][1].sh_offset = file_offset
            elements.append((writer, seg[1]))
            file_offset += len(seg[1])

        start = file_offset
        for name, shdr in shdrs:
            shdr.sh_name = file_offset - start
            elements.append((writer, name + b'\0'))
            file_offset += len(name) + 1
        shdr.sh_offset = start
        shdr.sh_size = file_offset - start

        for build, data in elements:
            build(data, f)

    def op_input_bin(self, f):
        ef = elftools.elf.elffile.ELFFile(f)
        if ef['e_machine'] != 'EM_AVR':
            raise Exception(f'Unexpected architecture: {ef["e_machine"]}')
        if ef['e_flags'] & 0x7f != self.mcuid:
            raise Exception(f'Unexpected mcuid in ELF Phdr flags: {ef.eflag & 0x7f}')
        segments = []
        for seg in ef.iter_segments('PT_LOAD'):
            if seg['p_filesz']:
                segments.append((seg['p_paddr'], seg.data()))
        return segments

formats = [FmtElf]
