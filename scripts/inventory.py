#!/usr/bin/env python3
"""Inventory an extracted app without executing it. Output contains paths/hashes,
Mach-O metadata, plist versions and file counts. Keep the report outside git.
Usage: python3 scripts/inventory.py APP --out /private/analysis/inventory.json
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import plistlib
import struct


def macho(data):
    """Read universal and little-endian thin Mach-O load commands and symbols."""
    if data[:4] == b'\xca\xfe\xba\xbe':
        count, = struct.unpack_from('>I',data,4)
        if count > 64 or 8+20*count > len(data): raise ValueError('invalid universal header')
        slices=[]
        for i in range(count):
            cpu,subtype,off,size,align=struct.unpack_from('>5I',data,8+20*i)
            if off+size>len(data): raise ValueError('truncated universal slice')
            item=macho(data[off:off+size])
            if item is None: item={'cpu':cpu,'subtype':subtype,'error':'unsupported slice format'}
            slices.append(item)
        return dict(format='universal',slices=slices)
    if data[:4] not in (b'\xce\xfa\xed\xfe', b'\xcf\xfa\xed\xfe'):
        return None
    wide = data[0] == 0xcf
    header_size = 32 if wide else 28
    _, cpu, subtype, kind, ncmds, sizeofcmds, flags = struct.unpack_from('<7I', data)
    if header_size + sizeofcmds > len(data):
        raise ValueError('truncated Mach-O load commands')
    result = dict(bits=64 if wide else 32, cpu=cpu, subtype=subtype,
                  filetype=kind, flags=flags, dependencies=[], segments=[], symbols=[])
    pos = header_size
    end = pos + sizeofcmds
    for _ in range(ncmds):
        cmd, size = struct.unpack_from('<II', data, pos)
        if size < 8 or pos + size > end:
            raise ValueError('invalid Mach-O command size')
        if cmd in (0xc, 0xd, 0x80000018, 0x8000001f):
            offset, = struct.unpack_from('<I', data, pos + 8)
            if offset >= size: raise ValueError('invalid dylib name offset')
            result['dependencies'].append(data[pos+offset:pos+size].split(b'\0')[0].decode('utf8', 'replace'))
        if cmd == 1:
            name, addr, vsize, off, fsize, maxprot, initprot, nsects, segflags = struct.unpack_from('<16s8I', data, pos+8)
            result['segments'].append(dict(name=name.rstrip(b'\0').decode(), address=addr,
                                            size=vsize, offset=off, file_size=fsize, protection=initprot))
        if cmd == 2:
            symoff, count, stroff, strsize = struct.unpack_from('<4I', data, pos+8)
            entry_size = 16 if wide else 12
            if symoff + count*entry_size > len(data) or stroff+strsize > len(data):
                raise ValueError('truncated Mach-O symbol table')
            strings = data[stroff:stroff+strsize]
            for i in range(count):
                idx, typ, sect, desc, addr = struct.unpack_from('<IBBHQ' if wide else '<IBBHI', data, symoff+i*entry_size)
                if idx >= strsize: raise ValueError('invalid symbol string offset')
                stop = strings.find(b'\0',idx)
                if stop == -1: raise ValueError('unterminated symbol name')
                name = strings[idx:stop].decode('utf8','replace')
                result['symbols'].append(dict(name=name,address=addr,type=typ,section=sect))
        pos += size
    if pos != end: raise ValueError('Mach-O command count/size mismatch')
    return result


def inventory(root):
    root = Path(root)
    if not root.is_dir(): raise ValueError('expected an extracted app directory')
    counts = collections.Counter()
    records, binaries, plists = [], {}, {}
    for path in sorted(root.rglob('*')):
        if path.is_symlink() or not path.is_file(): continue
        relative = path.relative_to(root).as_posix()
        counts[path.suffix.lower() or '(none)'] += 1
        with path.open('rb') as stream:
            digest = hashlib.file_digest(stream, 'sha256').hexdigest()
            stream.seek(0); magic = stream.read(4)
        records.append(dict(path=relative,size=path.stat().st_size,sha256=digest))
        if magic in (b'\xce\xfa\xed\xfe', b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe'):
            binaries[relative] = macho(path.read_bytes())
        if path.name == 'Info.plist':
            try: plists[relative] = plistlib.loads(path.read_bytes())
            except plistlib.InvalidFileException: plists[relative] = {'error':'invalid plist'}
    return dict(schema=1,file_count=len(records),extensions=dict(counts),files=records,
                binaries=binaries,plists=plists)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app', type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    report = inventory(args.app)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2,default=str)+'\n')
    print(f"Inventoried {report['file_count']} files and {len(report['binaries'])} Mach-O binaries")
