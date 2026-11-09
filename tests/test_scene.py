import importlib.util
from pathlib import Path
import struct
import unittest
import xml.etree.ElementTree as ET
import zlib

spec=importlib.util.spec_from_file_location('prepare_scene',Path(__file__).resolve().parents[1]/'scripts/prepare_scene.py')
scene=importlib.util.module_from_spec(spec); spec.loader.exec_module(scene)


class SceneTests(unittest.TestCase):
    def test_spr_png(self):
        palette=bytearray(768); palette[3:6]=b'\x10\x20\x30'
        data=b'SPR'+struct.pack('<3H',1,2,1)+struct.pack('<H',80)+palette+b'\0\1'
        png=scene.spr_first_png(data)
        self.assertEqual(png[:8],b'\x89PNG\r\n\x1a\n')
        pos=8; decoded=None
        while pos<len(png):
            length,=struct.unpack_from('>I',png,pos); kind=png[pos+4:pos+8]
            if kind==b'IDAT': decoded=zlib.decompress(png[pos+8:pos+8+length])
            pos+=12+length
        self.assertEqual(decoded,b'\0\0\0\0\0\x10\x20\x30\xff')
        with self.assertRaises(ValueError): scene.spr_first_png(data[:-1])

    def test_blockers_and_spawn(self):
        room=ET.fromstring('<room><row text="#####"/><row text="#.1.#"/><row text="#####"/><element at="1" type="foodvending"/></room>')
        visual,collision=scene.scene_rows(room)
        self.assertEqual(visual[1],'#.1.#')
        self.assertEqual(collision[1][2],'#')
        self.assertEqual(''.join(collision).count('@'),1)

    def test_unknown_symbols_rejected(self):
        room=ET.fromstring('<room><row text=".?"/></room>')
        with self.assertRaises(ValueError): scene.scene_rows(room)
