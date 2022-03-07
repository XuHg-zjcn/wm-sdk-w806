#!/bin/python3
######################################################################
'''W806串口调试的GDB远程调试服务器'''
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
import threading
from serdbg import BKPT_Mode


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
                self.__send('PacketSize=1000;multiprocess+;swbreak+;hwbreak+')
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
            elif recv == 's':
                self.ll.Step()
                self.__send('OK')
                self.ll.Wait_Step()
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
            self.conn, addr = self.socket.accept()
            print('Connect to', addr)
            self.Interactive()
