#!/bin/python3
######################################################################
'''W806串口调试的二进制通信协议'''
# Copyright (C) 2022  Xu Ruijun
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
######################################################################
import time
import struct
from enum import Enum


# 0x00-0x1f |  0-31 |  r0-31
# 0x24 | 36 | lo
# 0x25 | 37 | hi
# 0x28-0x37 | 40-55 | fr0-15
# 0x38-0x47 | 56-71 | vr0-15
# 0x48 | 72 | pc

# 0x59 | 89 | psr   | cr<0,0>
# 0x5a | 90 | vbr   | cr<1,0>
# 0x5b | 91 | epsr  | cr<2,0>
# 0x5d | 93 | epc   | cr<4,0>
# 0x66-0x78 | cr13-31

# 
# 0x
# 0x8c-0x8f | profcr0-4
# 0x90-0x9d | profsgr0-13
# 0xa0-0xae | profagr0-14
# 0xb0-0xbc | profxgr0-14
def reg_gdb2num(rx):
    if 0 <= rx < 32:
        pass
    elif 40 <= rx < 56:
        rx -= 8
    elif rx == 0x48:
        rx = 49
    elif rx == 0x59:
        rx = 48
    else:
        rx = -1 #ValueError(f'unsupported register r{rx}')
    return rx


class SerDbg_Cmd(Enum): # Param,                       | ret           
    Read_Reg = 0        # rx-ry(2B)                    | data(4nB)
    Write_Reg = 1       # rx-ry(2B), data(4nB)         | stat(1B)
    Read_Mem = 2        # addr(4B), len(2B)            | data(len), stat(1B)
    Write_Mem = 3       # addr(4B), len(2B), data(lenB)| stat(1B)
    Read_Flash = 4      #
    Write_Flash = 5     #
    New_BKPT = 6        # addr(4B), mode(1B)           | index(1B), old(2B), new(2B)
    Mode_BKPT = 7       # index(1B), set(1B)           | stat(1B)
    Get_BKPT = 8        # index(1B)                    | [n(1B)], set(nB)
    Pause = 9           #                              |
    Resume = 10         #                              |
    Step = 11           #                              |
    About = 12          #                              | str


class BKPT_Mode(Enum):
    BPNone = 0
    Ignore = 1
    BinCmd = 2
    StrNum = 3
    StrRegBase = 4
    StrRegAll = 5


class BKPTS:
    def __init__(self):
        self.d = {}    # key:addr, value:[index, mode, old]
        self.tmp = {}  # key:addr, value:mode_

    def add_bkpt(self, addr, index, mode, old=None):
        assert isinstance(addr, int)
        assert isinstance(index, int)
        assert isinstance(mode, BKPT_Mode)
        assert isinstance(old, (type(None), bytes))
        self.d[addr] = [index, mode, old]
    # remove bkpt: `set_mode(addr, BKPT_Mode.BPNone)`

    def set_mode(self, addr, mode_):
        assert isinstance(mode_, BKPT_Mode)
        if addr not in self.d:
            raise KeyError
        self.tmp[addr] = mode_

    def update(self):
        invalid = []
        for addr, mode_ in self.tmp.items():
            if self.d[addr][1] == mode_:
                invalid.append(addr)
        for k in invalid:
            self.tmp.pop(k)
        ret = []
        for addr, mode_ in self.tmp.items():
            index = self.d[addr][0]
            ret.append((index, mode_))
            if mode_ == BKPT_Mode.BPNone:
                self.d.pop(addr)
            else:
                self.d[addr][1] = mode_
        self.tmp.clear()
        return ret


class SerDbg:
    def __init__(self, ser, queue):
        self.ser = ser
        self.queue = queue
        self.pause = False
        self.bkpts = BKPTS()

    def __send_cmd(self, name, *args):
        #if not self.stat:
        if not self.pause:
            self.ser.write(b'AT+SDB\r\n')
            time.sleep(0.05)
        if name == 'Pause':
            self.pause = True
        if name == 'Resume':
            self.pause = False
        sf = '<B'
        sd = [SerDbg_Cmd[name].value]
        for c, data in args:
            sf += c
            sd.append(data)
        data = struct.pack(sf, *sd)
        print('w', data.hex())
        self.ser.write(data)

    def __send_dat(self, data):
        self.ser.write(data)

    def __recv(self, size):
        # TODO: pause模式下不再接收同步头
        g = self.queue.get()
        if g != b'\n':
            raise ValueError(g)
        data = self.ser.read(size)
        print('r', data.hex())
        self.queue.task_done()
        return data

    def Read_Regs(self):
        self.__send_cmd('Read_Reg', ('B', 0), ('B', 16))
        return self.__recv(4*16).hex()

    def Write_Regs(self):
        assert len(data) % 4 == 0, 'invaild reg numbers'
        self.__send_cmd('Write_Reg', ('B', 0), ('B', 16))
        self.__send_dat(4*16)

    def Read_aReg(self, rx):
        rx = reg_gdb2num(rx)
        if rx < 0:
            return "E05"
        self.__send_cmd('Read_Reg', ('B', rx), ('B', rx+1))
        return self.__recv(4).hex()

    def Write_aReg(self, rx, data):
        rx = reg_gdb2num(rx)
        self.__send_cmd('Write_Reg', ('B', rx), ('B', rx+1))
        self.__send_dat(data)

    def Read_Mem(self, addr, size):
        self.__send_cmd('Read_Mem', ('I', addr), ('H', size))
        return self.__recv(size).hex()

    def Write_Mem(self, addr, data):
        self.__send_cmd('Read_Mem', ('I', addr), ('H', len(data)))
        self.__send_dat(data)

    def New_BKPT(self, addr, mode=BKPT_Mode.BinCmd):
        if addr in self.bkpts.d:
            self.bkpts.set_mode(addr, mode)
            return
        self.__send_cmd('New_BKPT', ('I', addr), ('B', mode.value))
        res = self.__recv(5)
        index, old, new = struct.unpack('<BHH', res)
        self.bkpts.add_bkpt(addr, index, mode, res[1:3])
        return index, new == 0

    def Remove_BKPT(self, addr):
        # TODO: 断点可能用完，需要自动清理
        self.bkpts.set_mode(addr, BKPT_Mode.Ignore)

    def Pause(self):
        self.__send_cmd('Pause')
        self.pause = True

    def Resume(self):
        for index, mode in self.bkpts.update():
            self.Set_BKPT_Mode(index, mode)
        self.__send_cmd('Resume')
        self.pause = False

    def Wait_BKPT(self):
        g = self.queue.get()
        if g[0] != ord('B'):
            raise ValueError(g)
        n = int(g[1:-1])
        print('r', f'B{n}')
        self.pause = True
        self.__send_cmd('Pause')
        self.queue.task_done()
        return n

    def Set_BKPT_Mode(self, index, mode):
        if index in {-1, 'all'}:
            index = 0xff
        self.__send_cmd('Mode_BKPT', ('B', index), ('B', mode.value))
        return struct.unpack('B', self.__recv(1))[0]

    def Step(self):
        self.__send_cmd('Step')
        self.pause = False

    def Wait_Step(self):
        g = self.queue.get()
        if g[0] != ord('S'):
            raise ValueError(g)
        print('s')
        self.pause = True
        self.__send_cmd('Pause')
        self.queue.task_done()
