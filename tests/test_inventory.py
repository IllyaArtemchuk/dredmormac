import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('inventory', Path(__file__).resolve().parents[1]/'scripts/inventory.py')
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)


class InventoryTests(unittest.TestCase):
    def test_synthetic_macho(self):
        header = struct.pack('<7I',0xfeedface,7,3,2,1,24,0)
        command = struct.pack('<6I',2,24,52,1,64,6)
        symbol = struct.pack('<IBBHI',1,0xf,1,0,0x1234)
        result = inventory.macho(header+command+symbol+b'\0test\0')
        self.assertEqual(result['bits'],32)
        self.assertEqual(result['symbols'][0]['name'],'test')
        self.assertEqual(result['symbols'][0]['address'],0x1234)
        with self.assertRaises(ValueError): inventory.macho(header+command+symbol)

    def test_hashes_and_symlinks(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root/'data.xml').write_text('<room/>')
            (root/'link').symlink_to('/etc')
            result = inventory.inventory(root)
            self.assertEqual(result['file_count'],1)
            self.assertEqual(result['extensions'],{'.xml':1})
            self.assertEqual(len(result['files'][0]['sha256']),64)

    def test_non_macho(self):
        self.assertIsNone(inventory.macho(b'not a Mach-O'))

    def test_universal_slice(self):
        thin=struct.pack('<7I',0xfeedface,7,3,2,0,0,0)
        universal=struct.pack('>7I',0xcafebabe,1,7,3,28,len(thin),0)+thin
        result=inventory.macho(universal)
        self.assertEqual(result['slices'][0]['cpu'],7)
        with self.assertRaises(ValueError): inventory.macho(universal[:-1])
