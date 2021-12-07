#!/bin/python3
import sys
import serial
import socket
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
    Set_BKPT = 6        # addr(4B)                     | index(1B)
    Upd_BKPT = 7        # index(1B), set(1B)           | stat(1B)
    Get_BKPTS = 8       #                              | n(1B), set(nB)
    Unset_BKPT = 9      # index(1B)                    | stat(1B)
    Clear_BKPT = 10     #                              | stat(1B)
    Contin_BKPT = 11    #                              |
    About = 12          #                              | str
    EXIT = 14           #                              | stat(1B)

class SerDbg:
    def __init__(self, ser):
        self.ser = ser
        #self.stat = False
    
    def __send_cmd(self, name, *args):
        #if not self.stat:
        self.ser.write(b'AT+SDB\r\n')
        sf = '<B'
        sd = [SerDbg_Cmd[name].value]
        for c, data in args:
            sf += c
            sd.append(data)
        self.ser.write(struct.pack(sf, *sd))

    def __send_dat(self, data):
        self.ser.write(data)

    def __recv(self, size):
        data = self.ser.read(size)
        print('r', data.hex())
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



class GDBServer:
    def __init__(self, conn):
        self.conn = conn
        self.tmp = []

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

    def run(self):
        while True:
            recv = self.recv()
            print('>', recv)
            if recv.find('qSupported') >= 0:
                self.__send('PacketSize=1000;multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+')
            if recv.find('qXfer:features:read:target.xml:') >= 0:
                x = recv.find('xml:')
                self.send_file(recv[x+4:], 'target.xml')
            if recv == 'vMustReplyEmpty':
                self.__send('')
            if recv.find('Hg') >= 0:
                self.__send('OK')
            if recv.find('qL1200') >= 0:
                self.__send('')
            if recv == 'qTStatus':
                self.__send('T1')
            if recv == 'qTfV':
                self.__send('l')
            if recv == 'udebugprintport':
                self.__send('1234')
            if recv == '?':
                self.__send('S05')
            if recv == 'qfThreadInfo':
                self.__send('m0')
            if recv == 'qsThreadInfo':
                self.__send('l')
            if recv == 'Hc-1':
                self.__send('OK')
            if recv == 'qAttached':
                self.__send('1')
            if recv == 'qC':
                self.__send('QC00')
            if recv == 'qOffsets':
                self.__send('')
            if recv == 'g':
                self.__send(self.Read_Regs())
            if recv[0] == 'p':
                rx = int(recv[1:], base=16)
                data = self.Read_aReg(rx)
                self.__send(data)
            if recv[0] == 'm':
                addr, size = recv[1:].split(',')
                addr = int(addr, base=16)
                size = int(size, base=16)
                data = self.Read_Mem(addr, size)
                self.__send(data)
            if recv == 'qTfP':
                self.__send('l')
            if recv == 'qSymbol::':
                self.__send('OK')
            if recv == 'vCont?':
                self.__send('vCont;c')
            if recv == 'vCont;c':
                self.__send('OK')
            if recv == 'D':
                break

    def read_areg(self, rx):
        pass

    def read_mem(self, addr, size):
        pass


class SDBServer(SerDbg, GDBServer):
    def __init__(self, ser, soc):
        SerDbg.__init__(self, ser)
        GDBServer.__init__(self, soc)


if __name__ == '__main__':
    ser = serial.Serial('/dev/'+sys.argv[1], 115200)
    soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    soc.bind(('127.0.0.1', 3334))
    soc.listen(1)
    
    conn, addr = soc.accept()
    print(addr)
    rsp = SDBServer(ser, conn)
    rsp.run()
