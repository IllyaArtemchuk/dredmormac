#!/usr/bin/env python3
"""Original currency construction, rolls and wallet transfers.

Actual Item/Zorkmids constructors, Build, currency factory paths and wallet
methods execute. Allocation/string/rand/strcmp, tweak, sprite I/O, sound and
floating-text boundaries are supplied; UI/entity registration is excluded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import struct
import subprocess
from verify_dungeon import GeometryOracle


def signed(value):return value if value<0x80000000 else value-0x100000000
def float_bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]


class Currency(GeometryOracle):
    def __init__(self,binary):
        from unicorn import UC_HOOK_CODE
        super().__init__(binary)
        for address in [0xabf20,0x5f4b0,0x5b440]:self.machine.hook_add(UC_HOOK_CODE,self.boundary,begin=address,end=address)
        self.machine.hook_add(UC_HOOK_CODE,self.observe_constructor,begin=0x5ca02,end=0x5ca02)
        self.original.call('__Z12init_genrandm',0xd0ed)
        self.expected_mt=[self.original.call('__Z13genrand_int32v') for _ in range(8)]

    def observe_constructor(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP
        if self.created is not None:raise AssertionError('multiple currency constructors')
        self.created=self.read(machine.reg_read(UC_X86_REG_ESP)+4)[0]

    def boundary(self,machine,address,size,user):
        from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EIP
        if address not in [0xabf20,0x5f4b0,0x5b440,0x179b90,0x1cbfec]:return super().boundary(machine,address,size,user)
        esp=machine.reg_read(UC_X86_REG_ESP);returned,a,b,c,d,e,f=self.read(esp,7);result=a
        if address==0xabf20:
            if self.sprite is not None or self.cstring(c) or d or e or f:raise AssertionError('unexpected currency sprite load')
            self.sprite=[self.cstring(a),self.cstring(b)];result=self.original.scratch+0x2000
        elif address==0x5f4b0:self.sounds.append(self.cstring(a))
        elif address==0x5b440:self.floats.append(signed(a))
        elif address==0x179b90:
            if self.cstring(a)!='gold drop per level':raise AssertionError('unexpected currency tweak')
            result=self.gold_per_level
        else:
            left,right=self.cstring(a),self.cstring(b);result=(left>right)-(left<right)
        machine.reg_write(UC_X86_REG_EAX,result&0xffffffff);machine.reg_write(UC_X86_REG_ESP,esp+4);machine.reg_write(UC_X86_REG_EIP,returned)

    def prepare(self,skills=(),abilities=()):
        self.heap=0x73000000;self.sprite=None;self.created=None;self.sounds=[];self.floats=[]
        self.actor=self.allocate(0x900);vtable=self.allocate(0x110);self.write(self.actor,vtable)
        self.write(vtable+0xf8,0x11c446,0x11c402);self.write(self.original.address('_curPlayer'),self.actor)
        for i,(identifier,rank) in enumerate(skills[:7]):self.write(self.actor+0x70c+i*4,identifier);self.write(self.actor+0x754+i*4,rank)
        entries=self.allocate(len(abilities)*4)
        for i,(skill,level,bonus) in enumerate(abilities):
            ability=self.allocate(0x210);self.write(ability+0x18,level,skill);self.write(ability+0x130,bonus);self.write(entries+i*4,ability)
        self.write(self.original.address('_abilityDB'),entries,entries+len(abilities)*4,entries+len(abilities)*4,0)
        self.level=self.allocate(0x100);self.write(self.original.address('_curLevel'),self.level)
        self.original.call('__Z12init_genrandm',0xd0ed)

    def check_mt(self):
        if [self.original.call('__Z13genrand_int32v') for _ in range(8)]!=self.expected_mt:raise AssertionError('currency unexpectedly consumed MT RNG')

    def object_output(self):
        if self.created is None or self.sprite is None:raise AssertionError('incomplete currency object')
        pointer=self.created;stack,quantity=self.read(pointer+0x98,2)
        self.check_mt()
        return [self.read(pointer+0x74)[0],signed(self.read(pointer+0x1c8)[0]),signed(quantity),stack,*self.sprite]

    def make(self,amount,skills,abilities):
        self.prepare(skills,abilities);pointer=self.allocate(0x1cc)
        self.original.call('__ZN8ZorkmidsC2Ei',pointer,amount,budget=1000000)
        if pointer!=self.created:raise AssertionError('currency constructor object differs')
        return self.object_output()

    def roll(self,seed,depth,alternate,alternate_depth,quantity):
        self.prepare();self.state=seed;self.write(self.level+0x64,depth,0,alternate,alternate_depth)
        # This factory's currency branch uses curLevel, not the Level argument.
        other=self.allocate(0x100);self.write(other+0x64,99)
        types=self.allocate(8);self.string(types,'zorkmids');self.string(types+4,'random')
        pointer=self.original.call('__Z35CreateItemFromTypeAndSubtypeStringsSsSsP5Levelib',types,types+4,other,quantity,0,budget=1000000)
        if pointer!=self.created:raise AssertionError('currency factory object differs')
        return self.object_output()+[self.state]

    def named(self,seed,depth,per_level,quantity):
        self.prepare();self.state=seed;self.gold_per_level=per_level;self.write(self.level+0x64,depth)
        # Real outer factory executes CreateItem(name), followed by its override.
        types=self.allocate(8);self.string(types,'unknown');self.string(types+4,'zorkmids')
        pointer=self.original.call('__Z35CreateItemFromTypeAndSubtypeStringsSsSsP5Levelib',types,types+4,self.level,quantity,0,budget=1000000)
        if pointer!=self.created:raise AssertionError('named outer currency factory object differs')
        return self.object_output()+[self.state]

    def transfer(self,operation,balance,amount):
        self.prepare();self.write(self.actor+0x58c,balance)
        self.original.call('__ZN6Player12GiveZorkmidsEi' if operation=='give' else '__ZN6Player12TakeZorkmidsEi',self.actor,amount,budget=100000)
        if len(self.floats)!=1 or self.sounds not in [[],['money_get']]:raise AssertionError('unexpected wallet effects')
        self.check_mt()
        return [signed(self.read(self.actor+0x58c)[0]),self.floats[0],int(bool(self.sounds))]


def verify(binary,port):
    oracle=Currency(binary);rng=random.Random(0x5ca02);commands=[];expected=[];counts=dict(create=0,roll=0,named=0,transfer=0)
    thresholds=[-2147483648,-1,0,1,9,10,49,50,249,250,999,1000,3999,4000,2147483647]
    for index in range(1040):
        amount=thresholds[index] if index<len(thresholds) else rng.randint(-2147483648,2147483647)
        skills=[(rng.randrange(12),rng.randrange(5)) for _ in range(7)]
        abilities=[(rng.randrange(16),rng.randint(-1,5),float_bits(rng.choice([0.,.1,.25,.5,1.,-1.,1e-30,1e30,float('inf'),float('nan')]))) for _ in range(rng.randrange(12))]
        expected.append(oracle.make(amount,skills,abilities));counts['create']+=1
        commands.append(' '.join(map(str,['create',amount,1,len(skills)]+[v for row in skills for v in row]+[len(abilities)]+[v for row in abilities for v in row])))
    for index in range(1024):
        seed=rng.getrandbits(32);depth=rng.choice([-10,-1,0,1,9,20,2147483647]);alternate=rng.randrange(2)
        alternate_depth=rng.choice([-1,0,5,20,2147483647]);quantity=rng.choice([-1,0,1,2,10,2147483647])
        expected.append(oracle.roll(seed,depth,alternate,alternate_depth,quantity));counts['roll']+=1
        commands.append(' '.join(map(str,['roll',seed,depth,alternate,alternate_depth,quantity])))
        per_level=rng.choice([-1,0,1,2,10,100,2147483647])
        expected.append(oracle.named(seed,depth,per_level,quantity));counts['named']+=1
        commands.append(' '.join(map(str,['named',seed,depth,per_level,quantity])))
    transfers=[(operation,balance,amount) for operation in ['give','take'] for balance in [0,-2147483648,2147483647] for amount in [-2147483647,-1,0,1,2147483647]]
    transfers.extend((rng.choice(['give','take']),rng.randint(-2147483648,2147483647),rng.randint(-2147483647,2147483647)) for _ in range(2048))
    for operation,balance,amount in transfers:
        expected.append(oracle.transfer(operation,balance,amount));counts['transfer']+=1;commands.append(f'{operation} {balance} {amount}')
    process=subprocess.run([str(port.resolve())],input='\n'.join(commands)+'\n',text=True,capture_output=True,check=True)
    actual=[shlex.split(line) for line in process.stdout.splitlines()];expected=[list(map(str,row)) for row in expected]
    if actual!=expected:
        index=next((i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'currency mismatch at fixture {index}: {actual[index]} / {expected[index]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=len(expected),groups=counts,mismatches=0,
                functions=['0x5ca02','0x18ac20','0x185bb0','0x11c402','0x11c446'],
                scope='actual currency constructors/Build, both currency factory paths/quantity override and wallet methods; controlled skills/ability records/levels, allocation/string/rand/strcmp/tweak/sprite/sound/floating-text boundaries; amount, sprite selection, Item quantity/stackability, wallet balance/effects, roll libc RNG and unchanged next eight MT outputs compared; INT_MIN transfer recursion and original XML metadata loading excluded')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path)
    parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();report=verify(args.binary,args.port)
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} original currency construction/roll/transfer cases match")
