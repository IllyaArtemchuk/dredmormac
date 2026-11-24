#!/usr/bin/env python3
"""Actual ordinary type0 lever interactions and activation history preservation."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shlex
import subprocess
from verify_levers import Levers


class ScriptLevers(Levers):
    def run(self,values):
        active,previous,older,tutorial,mode,x,y,px,py,held=values
        self.heap=0x73000000;self.messages=[];self.sound='';self.graphics=[];self.tutorial=False;self.state=12345
        self.level=self.allocate(0x100);lever=self.allocate(0x100);player=self.allocate(0x900)
        self.write(self.original.address('_curLevel'),self.level);self.write(self.original.address('_curPlayer'),player);self.write(self.original.address('_tutorialFlag'),tutorial);self.write(0x59f28+0x20d73c,held)
        self.write(lever,self.original.address('__ZTV5Lever')+8);self.write(lever+0x50,x,y);self.write(lever+0x74,active,previous,older);self.write(lever+0x80,0);self.write(player+0x50,px,py)
        self.original.call('__Z12init_genrandm',12345);before=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4));index=self.read(self.original.address('__ZL3mti'))
        symbol=['__ZN5Lever9FlipLeverEv','__ZN5Lever7ClickedEv','__ZN5Lever16RemoteActivationEv','__ZN5Lever21PlayerTriesToMoveIntoEv'][mode]
        handled=self.original.call(symbol,lever,budget=500000)
        if self.state!=12345 or before!=bytes(self.machine.mem_read(self.original.address('__ZL2mt'),624*4)) or index!=self.read(self.original.address('__ZL3mti')):raise AssertionError('ordinary lever consumed RNG')
        history=[v if v<2**31 else v-2**32 for v in self.read(lever+0x74,3)]
        if self.graphics:raise AssertionError('ordinary lever generated wall graphics')
        return list(map(str,[handled,*history,int(self.tutorial),len(self.messages),*self.messages,self.sound]))


def verify(binary,port,count):
    oracle=ScriptLevers(binary);rng=random.Random(0x7bcfd);lines=[];expected=[];ints=[-2147483648,-1,0,1,2,2147483647]
    for i in range(count):
        x,y=rng.randrange(-20,21),rng.randrange(-20,21);values=[rng.choice(ints),rng.choice(ints),rng.choice(ints),rng.randrange(2),i%4,x,y,x+rng.randrange(-2,3),y+rng.randrange(-2,3),rng.randrange(2)]
        lines.append(' '.join(map(str,values)));expected.append(oracle.run(values))
    process=subprocess.run([str(port.resolve()),'--scripted'],input='\n'.join(lines)+'\n',capture_output=True,text=True,check=True);actual=[shlex.split(line) for line in process.stdout.splitlines()]
    if actual!=expected:
        i=next((j for j,(a,b) in enumerate(zip(actual,expected)) if a!=b),min(len(actual),len(expected)))
        raise AssertionError(f'ordinary lever fixture{i}: {actual[i]}/{expected[i]}: {lines[i]}')
    return dict(binary_sha256=hashlib.sha256(oracle.original.data).hexdigest(),cases=count,mismatches=0,scope='Actual full type0 Lever::FlipLever/Clicked/RemoteActivation/PlayerTriesToMoveInto, actual cardinal/held-item gates and Activatable virtuals. Current/previous/older state, tutorial activation-before-toggle, messages, sound and absent graphics/no libc or MT consumption compared. Ticker/sound/tutorial boundaries supplied; constructor/resources, other lever types, Session scheduling/save and room script execution separate.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('binary',type=Path);parser.add_argument('port',type=Path);parser.add_argument('--cases',type=int,default=8192);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();report=verify(args.binary,args.port,args.cases);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['cases']} complete ordinary lever interactions match")
