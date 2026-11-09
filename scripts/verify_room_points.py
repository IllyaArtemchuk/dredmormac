#!/usr/bin/env python3
"""Original PointProperty execution with library boundaries and tiny attribute maps.

Original tree lookup, coordinate decisions and template mutation execute. String
operations, ASCII tolower/atoi and erase of at most two map nodes are supplied.
"""
import struct
from pathlib import Path
from verify_dungeon import GeometryOracle
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX

class Points(GeometryOracle):

    def __init__(self, p):
        super().__init__(p)
        a = self.original.address('__Z14roomParseErrorPKcz')
        self.machine.hook_add(UC_HOOK_CODE, self.boundary, begin=a, end=a)

    def boundary(self, m, a, size, user):
        esp = m.reg_read(UC_X86_REG_ESP)
        ret, x, y, z = self.read(esp, 4)
        if a in [0x1cbc20, 0x1cbcaa]:
            v = self.read(x)[0] + len(self.value(x))
        elif a in [0x1cbc38, 0x1cbcbc]:
            v = self.read(x)[0]
        elif a == 0x1cbe66:
            import re
            match = re.match('[ \\t\\n\\r\\f\\v]*([+-]?\\d+)', self.cstring(x))
            v = int(match.group(1)) if match else 0
        elif a == 0x1cbe48:
            v = x + 32 if 65 <= x <= 90 else x
        elif a == 0x1cbfda:
            l, r = (self.cstring(x).lower(), self.cstring(y).lower())
            v = (l > r) - (l < r)
        elif a == 0x1cbd34:
            v = self.read(x)[0] + y
        elif a == 0x1cbda0:
            parent, left, right = self.read(x + 4, 3)
            replacement = left or right
            if left and right:
                raise RuntimeError('two-child test erase unsupported')
            if parent == y:
                self.write(y + 4, replacement)
            elif self.read(parent + 8)[0] == x:
                self.write(parent + 8, replacement)
            else:
                self.write(parent + 12, replacement)
            if replacement:
                self.write(replacement + 4, parent)
            root = self.read(y + 4)[0]
            for offset, edge in [(8, 8), (12, 12)]:
                node = root
                while node and self.read(node + edge)[0]:
                    node = self.read(node + edge)[0]
                self.write(y + offset, node or y)
            v = x
        elif a == self.original.address('__Z14roomParseErrorPKcz'):
            raise RuntimeError('PARSE ERROR ' + self.cstring(x))
        else:
            return super().boundary(m, a, size, user)
        m.reg_write(UC_X86_REG_EAX, v & 4294967295)
        m.reg_write(UC_X86_REG_ESP, esp + 4)
        m.reg_write(UC_X86_REG_EIP, ret)

    def setup(self, prefix, attrs):
        point = self.allocate(28)
        self.string(point + 4, prefix)
        dest = self.allocate(8)
        self.write(dest, 71, 72)
        self.write(point + 20, dest, dest + 4)
        table = self.allocate(24)
        self.write(table, 0, 0, 0, table + 4, table + 4, len(attrs))
        nodes = []
        for k, v in sorted(attrs.items(), key=lambda t: t[0].lower()):
            n = self.allocate(24)
            self.string(n + 16, k)
            self.string(n + 20, v)
            nodes.append(n)
        if nodes:
            self.write(table + 8, nodes[0], nodes[0], nodes[-1])
            self.write(nodes[0], 1, table + 4, 0, nodes[1] if len(nodes) > 1 else 0)
            if len(nodes) > 1:
                self.write(nodes[1], 0, nodes[0], 0, 0)
        ats = self.original.address('_curAts')
        node = self.allocate(28)
        self.write(ats, 0, 0, node, node, node, 1)
        self.write(node, 1, ats + 4, 0, 0, ord('3'), 2, 4)
        template = self.allocate(132)
        self.write(template, 7, 7)
        cells = self.allocate(49)
        self.write(template + 24, cells)
        self.write(self.original.address('_curTemplate'), template)
        result = self.original.call('__ZN13PointProperty7SetFromERSt3mapISsSs7ci_lessSaISt4pairIKSsSsEEE', point, table, budget=1000000)
        return (result, self.read(dest, 2), list(self.machine.mem_read(cells, 49)), self.read(table + 20)[0])


def verify(binary,port):
    import hashlib
    import json
    import random
    import subprocess
    oracle=Points(binary);rng=random.Random(0x1ace5e);expected=[];lines=[]
    for index in range(1024):
        prefix=['','target','foo'][index%3];mode=index%5
        if mode==0:attributes={prefix+'at':'3'}
        else:
            attributes={}
            if mode in (1,3):attributes[prefix+'x']=rng.choice([str(rng.randint(-10000,10000)),' +2tail','invalid','  -9'])
            if mode in (1,4):attributes[prefix+'y']=rng.choice([str(rng.randint(-10000,10000)),' 4suffix','', '+0'])
        attributes={key.upper() if rng.randrange(2) else key:value for key,value in attributes.items()}
        result,position,cells,remaining=oracle.setup(prefix,attributes)
        position=[v if v<0x80000000 else v-0x100000000 for v in position]
        expected.append(list(map(str,[result]+position+[remaining]+cells)))
        values=[json.dumps(prefix),str(len(attributes))]
        for key,value in attributes.items():values.extend([json.dumps(key),json.dumps(value)])
        lines.append(' '.join(values))
    process=subprocess.run([str(port.resolve())],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True)
    actual=[line.split() for line in process.stdout.splitlines()]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'room point mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),function='0x1ace5e',cases=len(expected),mismatches=0,
                scope='full original PointProperty::SetFrom on zero/one/two-node synthetic ASCII attribute maps and one-marker table; string, ASCII tolower/atoi and map erase boundaries supplied; found/missing result, coordinates, remaining attr count and numeric template cells compared; malformed parse errors excluded')


if __name__=='__main__':
    import argparse
    import json
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original room point lookups/template mutations match")
