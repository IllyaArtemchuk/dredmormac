import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from original_i386 import OriginalI386


@unittest.skipUnless(importlib.util.find_spec('unicorn'),'Unicorn optional for original-routine execution')
class OriginalTests(unittest.TestCase):
    def test_distinct_floating_call_targets(self):
        # A synthetic Mach-O containing our own two three-byte x87 functions.
        # Alternating targets catches stale translated call sites in the oracle.
        data=bytearray(170)
        data[:28]=struct.pack('<7I',0xfeedface,7,3,2,2,80,0)
        data[28:84]=struct.pack('<II16s8I',1,56,b'__TEXT',0x1000,4096,0,len(data),7,7,0,0)
        data[84:108]=struct.pack('<6I',2,24,136,2,160,10)
        data[128:134]=b'\xd9\xe8\xc3\xd9\xee\xc3'  # fld1/ret; fldz/ret
        data[136:148]=struct.pack('<IBBHI',1,0xf,1,0,0x1080)
        data[148:160]=struct.pack('<IBBHI',5,0xf,1,0,0x1083)
        data[160:170]=b'\0one\0zero\0'
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'synthetic';path.write_bytes(data)
            original=OriginalI386(path)
            for _ in range(4):
                self.assertEqual(original.call('one',floating=True),0x3ff0000000000000)
                self.assertEqual(original.call('zero',floating=True),0)
