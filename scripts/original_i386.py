"""Bounded original-routine execution, not a macOS process emulator.

Loads the user's thin Mach-O into Unicorn. Resolves named routines and passes
cdecl arguments. An optional harness thunk stores ST0 as a double; those thunk
instructions are generated here, never taken from the game. No imports are stubbed.
"""
import struct
from inventory import macho


class OriginalI386:
    def __init__(self, binary):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
        from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_MXCSR
        self.data = binary.read_bytes()
        self.info = macho(self.data)
        if not self.info or self.info.get('bits') != 32 or self.info['cpu'] != 7:
            raise ValueError('expected thin i386 Mach-O')
        self.machine = Uc(UC_ARCH_X86, UC_MODE_32)
        self.machine.reg_write(UC_X86_REG_FPCW,0x37f)
        self.machine.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for segment in self.info['segments']:
            if not segment['file_size']: continue
            self.machine.mem_map(segment['address'],(segment['size']+4095)&~4095)
            self.machine.mem_write(segment['address'],self.data[segment['offset']:segment['offset']+segment['file_size']])
        self.stack, self.sentinel, self.scratch = 0x70000000, 0x71000000, 0x72000000
        self.machine.mem_map(self.stack,0x10000)
        self.machine.mem_map(self.sentinel,0x1000)
        self.machine.mem_map(self.scratch,0x10000)
        self.thunks={}

    def address(self, symbol):
        return next(s['address'] for s in self.info['symbols'] if s['name']==symbol and s['address'])

    def call(self, symbol, *args, floating=False, budget=200000):
        from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP
        address=self.address(symbol)
        esp=self.stack+0xff00
        self.machine.mem_write(esp,struct.pack('<'+'I'*(len(args)+1),self.sentinel,*[a&0xffffffff for a in args]))
        self.machine.reg_write(UC_X86_REG_ESP,esp)
        if floating:
            if args: raise ValueError('floating thunk supports no arguments')
            if symbol not in self.thunks:
                thunk=self.sentinel+0x100+len(self.thunks)*0x20
                if thunk+12>=self.sentinel+0x1000: raise ValueError('too many floating thunks')
                # Each routine gets its own immutable call site. Rewriting a call
                # in place can leave a cached Unicorn translation pointing at
                # the previous target, making an incorrect reference oracle.
                code=b'\xe8'+struct.pack('<i',address-(thunk+5))+b'\xdd\x1d'+struct.pack('<I',self.scratch)+b'\xc3'
                self.machine.mem_write(thunk,code)
                self.thunks[symbol]=thunk
            address=self.thunks[symbol]
        self.machine.emu_start(address,self.sentinel,count=budget)
        if self.machine.reg_read(UC_X86_REG_EIP)!=self.sentinel:
            raise RuntimeError(f'{symbol} did not return within instruction budget')
        if floating:
            return struct.unpack('<Q',self.machine.mem_read(self.scratch,8))[0]
        return self.machine.reg_read(UC_X86_REG_EAX)
