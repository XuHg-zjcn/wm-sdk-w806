#!/bin/python3
######################################################################
'''W806串口调试的串口监视器，过滤用于调试二进制数据'''
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
import threading


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
