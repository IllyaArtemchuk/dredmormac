#!/usr/bin/env python3
"""Compare every SPR frame against an independent palette expansion.
This validates asset decoding, not original engine animation scheduling.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess


def compare(root, executable):
    files=frames=0
    for path in sorted(root.rglob('*.spr')):
        data=path.read_bytes()
        if data[:3]!=b'SPR': raise ValueError('SPR signature mismatch')
        count,width,height=struct.unpack_from('<HHH',data,3)
        pixels=width*height
        if not count or not pixels or len(data)!=9+count*(770+pixels):
            raise ValueError('unexpected SPR layout')
        fingerprint=14695981039346656037
        for frame in range(count):
            start=9+frame*(770+pixels)
            palette=[data[start+2+3*i:start+5+3*i]+bytes([255 if i else 0]) for i in range(256)]
            rgba=b''.join(palette[i] for i in data[start+770:start+770+pixels])
            for byte in data[start:start+2]+rgba:
                fingerprint=((fingerprint^byte)*1099511628211)&0xffffffffffffffff
        actual=json.loads(subprocess.check_output([str(executable),str(path)],text=True))
        expected=dict(width=width,height=height,frames=count,hash=f'{fingerprint:x}')
        if actual!=expected: raise AssertionError(f'sprite comparison failed: {path}')
        files+=1;frames+=count
    if not files: raise ValueError('no SPR files found')
    return dict(files=files,frames=frames,mismatches=0,scope='palette decoding; index-zero transparency assumption; timing unverified')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data_root',type=Path)
    parser.add_argument('--executable',type=Path,default=Path('build/dredmor-sprite'))
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    report=json.dumps(compare(args.data_root,args.executable.resolve()),indent=2)+'\n'
    if args.report: args.report.write_text(report)
    print(report,end='')
