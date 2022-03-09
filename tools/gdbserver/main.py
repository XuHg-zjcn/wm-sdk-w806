#!/bin/python3
######################################################################
'''W806串口调试的main文件'''
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
import sys
import serial
import socket
import signal
import queue
from sermon import SerMon
from serdbg import SerDbg, BKPT_Mode
from gdbserver import GDBServer


def pexit(signum, frame):
    print('exit')
    if len(sdb.bkpts.d) != 0:
        sdb.Set_BKPT_Mode('all', BKPT_Mode.BPNone)
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
