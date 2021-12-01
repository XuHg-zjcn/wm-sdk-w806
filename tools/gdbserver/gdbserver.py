#!/bin/python3
import sys
import serial
import socket
import struct


class SerDbg:
    def __init__(self, ser):
        self.ser = ser
        #self.stat = False
    
    def __send(self, data):
        #if not self.stat:
        self.ser.write(b'AT+SDB\r\n')
        self.ser.write(data)

    def __recv(self, size):
        return self.ser.read(size)
    
    def read_areg(self, rx):
        if 0 <= rx < 32:
            pass
        elif 53 <= rx < 69:
            rx -= 21
        elif rx == 72:
            rx = 50
        else:
            raise ValueError('unsupported register')
        self.__send(bytes([2, rx]))
        return self.__recv(4)

    def read_mem(self, addr, size):
        self.__send(struct.pack('<BIH', 4, addr, size))
        return self.__recv(size)


class GDBServer:
    def __init__(self, conn):
        self.conn = conn
        self.tmp = []

    def recv(self):
        while not self.tmp:
            recv = self.conn.recv(1024)
            l = recv.split(b'$')
            l = list(map(lambda x:x.rstrip(b'+').rstrip(b'-'), l))
            while b'' in l:
                l.remove(b'')
            for s in l:
                s2 = s.split(b'#')
                if len(s2) == 0:
                    continue
                assert len(s2) == 2, f"can't cut checksum, {s2}"
                data, chksum = s2
                dsum = sum(data)&0xff
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
        sumx = sum(data)&0xff
        data = b'+$' + data + (f'#{sumx:02x}').encode()
        self.conn.send(data)

    def send_file(self, cmd, fn):
        print(cmd)
        p, s = cmd.split(b',')
        p = int(p, base=16)
        s = int(s, base=16)
        with open(fn, 'rb') as f:
            while True:
                data = f.read(s)
                if len(data) == s:
                    self.__send(b'm'+data)
                else:
                    self.__send(b'l'+data)
                    break
                # TODO: 完善多次读取

    def run(self):
        while True:
            recv = self.recv()
            print('>', recv)
            if recv.find(b'qSupported') >= 0:
                self.__send(b'PacketSize=1000;qXfer:features:read+')
            if recv.find(b'qXfer:features:read:target.xml:') >= 0:
                x = recv.find(b'xml:')
                self.send_file(recv[x+4:], 'target.xml')
            if recv == b'vMustReplyEmpty':
                self.__send(b'')
            if recv.find(b'Hg') >= 0:
                self.__send(b'OK')
            if recv.find(b'qL1200') >= 0:
                self.__send(b'')
            if recv == b'qTStatus':
                self.__send(b'T0')
            if recv == b'qTfV':
                self.__send(b'l')
            if recv == b'udebugprintport':
                self.__send(b'1234')
            if recv == b'?':
                self.__send(b'S05')
            if recv == b'qfThreadInfo':
                self.__send(b'm0')
            if recv == b'qsThreadInfo':
                self.__send(b'l')
            if recv == b'Hc-1':
                self.__send(b'OK')
            if recv == b'qAttached':
                self.__send(b'1')
            if recv == b'qC':
                self.__send(b'QC00')
            if recv == b'qOffsets':
                self.__send(b'')
            if recv == b'g':
                self.__send(b'x'*8*50)
            if recv[0] == ord('p'):
                rx = int(recv[1:], base=16)
                data = self.read_areg(rx)
                self.__send(data.hex().encode())
            if recv[0] == ord('m'):
                addr, size = recv[1:].split(b',')
                addr = bytes.fromhex(addr)
                addr = int.from_bytes(addr, 'little')
                size = int(size, base=16)
                data = self.read_mem(addr, size)
                self.__send(data.hex())
            if recv == b'qTfP':
                self.__send(b'l')
            if recv == b'qSymbol::':
                self.__send(b'OK')

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
    soc.bind(('127.0.0.1', 3335))
    soc.listen(1)
    
    conn, addr = soc.accept()
    print(addr)
    rsp = SDBServer(ser, conn)
    rsp.run()
