#!/bin/python3
######################################################################
'''W806串口调试协议转换为GDB远程调试协议(RSP)'''
# Copyright (C) 2021-2022  Xu Ruijun
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
import sys
import time
import serial
import socket
import struct
import signal
import threading
import queue
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
    About = 11          #                              | str


class BKPT_Mode(Enum):
    BPNone = 0
    Ignore = 1
    BinCmd = 2
    StrNum = 3
    StrRegBase = 4
    StrRegAll = 5


class SerMon(threading.Thread):
    def __init__(self, ser, queue):
        self.ser = ser
        self.queue = queue
        super().__init__()

    def run(self):
        sync = b'\n+SDB:'
        i = 0
        while True:
           r = self.ser.read(1)
           if ord(r) == sync[i]:
               i += 1
               if i == len(sync):
                   i = 0
                   line = self.ser.readline()
                   self.queue.put(line)
                   self.queue.join()
           else:
               try:
                   s = (sync[:i]+r).decode()
               except Exception:
                   continue
               sys.stdout.write(s)
               sys.stdout.flush()
               i = 0


class SerDbg:
    def __init__(self, ser, queue):
        self.ser = ser
        self.queue = queue
        self.pause = False

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

    def New_BKPT(self, addr):
        self.__send_cmd('New_BKPT', ('I', addr), ('B', BKPT_Mode.BinCmd.value))
        index, old, new = struct.unpack('<BHH', self.__recv(5))
        return index, new == 0

    def Pause(self):
        self.__send_cmd('Pause')
        self.pause = True

    def Resume(self):
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
        return struct.unpack('B', self.__recv(1))


class GDBServer(threading.Thread):
    def __init__(self, socket, ll):
        self.socket = socket
        self.conn = None
        self.ll = ll
        self.tmp = []
        super().__init__()

    def recv(self):
        while not self.tmp:
            recv = self.conn.recv(1024).decode()
            l = recv.split('$')
            l = list(map(lambda x:x.rstrip('+').rstrip('-'), l))
            while '' in l:
                l.remove('')
            for s in l:
                s2 = s.split('#')
                if len(s2) == 0:
                    continue
                assert len(s2) == 2, f"can't cut checksum, {s2}"
                data, chksum = s2
                dsum = sum(data.encode())&0xff
                csum = int(chksum, base=16)
                if dsum != csum:
                    raise ValueError('checksum error')
                else:
                    self.tmp.append(data)
        return self.tmp.pop(0)

    def send_raw(self, data):
        self.conn.send(data)

    def __send(self, data):
        print('<', data)
        sumx = sum(data.encode())&0xff
        data = f'+${data}#{sumx:02x}'
        self.conn.send(data.encode())

    def send_file(self, cmd, fn):
        print(cmd)
        p, s = cmd.split(',')
        p = int(p, base=16)
        s = int(s, base=16)
        with open(fn, 'r') as f:
            while True:
                data = f.read(s)
                if len(data) == s:
                    self.__send('m'+data)
                else:
                    self.__send('l'+data)
                    break
                # TODO: 完善多次读取

    def Interactive(self):
        while True:
            recv = self.recv()
            print('>', recv)
            if recv.find('qSupported') >= 0:
                self.__send('PacketSize=1000;multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+')
            elif recv.find('qXfer:features:read:target.xml:') >= 0:
                x = recv.find('xml:')
                self.send_file(recv[x+4:], 'target.xml')
            elif recv == 'vMustReplyEmpty':
                self.__send('')
            elif recv.find('Hg') >= 0:
                self.__send('OK')
            elif recv.find('qL1200') >= 0:
                self.__send('')
            elif recv == 'qTStatus':
                self.__send('T1')
            elif recv == 'qTfV':
                self.__send('l')
            elif recv == 'udebugprintport':
                self.__send('1234')
            elif recv == '?':
                self.__send('S05')
            elif recv == 'qfThreadInfo':
                self.__send('m0')
            elif recv == 'qsThreadInfo':
                self.__send('l')
            elif recv.find('qAttached') == 0:
                self.__send('1')
            elif recv == 'Hc-1':
                self.ll.Pause()
                self.__send('OK')
            elif recv == 'Hcpa410.0':
                self.__send('OK')
            elif recv == 'c':
                self.ll.Resume()
                self.__send('OK')
                self.ll.Wait_BKPT()
                self.__send('S05')
            elif recv == 'qC':
                self.__send('QC00')
            elif recv == 'qOffsets':
                self.__send('')
            elif recv == 'g':
                self.__send(self.ll.Read_Regs())
            elif recv[0] == 'p':
                rx = int(recv[1:], base=16)
                data = self.ll.Read_aReg(rx)
                self.__send(data)
            elif recv[0] == 'm':
                addr, size = recv[1:].split(',')
                addr = int(addr, base=16)
                size = int(size, base=16)
                data = self.ll.Read_Mem(addr, size)
                self.__send(data)
            elif recv[0] == 'Z':
                Type, addr, size = recv[1:].split(',')
                addr = int(addr, base=16)
                self.ll.New_BKPT(addr)
                self.__send('OK')
            elif recv[0] == 'z':
                self.__send('OK')
            elif recv == 'qTfP':
                self.__send('l')
            elif recv == 'qSymbol::':
                self.__send('OK')
            elif recv == 'vCont?':
                self.__send('vCont;c')
            elif recv == 'vCont;c':
                self.__send('OK')
            elif recv[0] == 'D':
                self.ll.Set_BKPT_Mode('all', BKPT_Mode.BPNone)
                self.ll.Resume()
                self.__send('OK')
                break

    def run(self):
        while True:
            self.conn, addr = soc.accept()
            print('Connect to', addr)
            self.Interactive()


def pexit(signum, frame):
    print('exit')
    if sdb.pause:
        sdb.Resume()
    exit()

'''
如果遇到端口被占用，请尝试先退出gdb后再启动本程序
'''
if __name__ == '__main__':
    signal.signal(signal.SIGINT, pexit)
    signal.signal(signal.SIGTERM, pexit)
    ser = serial.Serial('/dev/'+sys.argv[1], 115200)
    soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    queue = queue.Queue(1)
    mon = SerMon(ser, queue)
    mon.start()
    sdb = SerDbg(ser, queue)
    soc.bind(('127.0.0.1', 3334))
    soc.listen(1)
    gdb = GDBServer(soc, sdb)
    gdb.start()
