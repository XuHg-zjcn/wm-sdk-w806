#!/usr/bin/python3
import time
import unittest
import serial
import queue
from sermon import SerMon
from serdbg import SerDbg, BKPT_Mode


class Test(unittest.TestCase):
    def test_1_ReadRegs(self):
        for i in range(5):
            sdb.Read_Regs()
            time.sleep(0.1)

    def BKPT_loop(self, epc, bkpti, N=5):
        for i in range(N):
            self.assertEqual(sdb.Wait_BKPT(), bkpti)
            self.assertEqual(sdb.Read_aReg(0x48), epc)
            sdb.Resume()

    def test_2_Step(self):
        sdb.Pause()
        for i in range(20):
            sdb.Step()
            sdb.Wait_Step()
        sdb.Resume()

    def test_3_BKPT(self):
        bkpt_addr = 0x0801aa8a
        bkpt_lb = int.to_bytes(bkpt_addr, 4, 'little').hex()
        i1, o = sdb.New_BKPT(bkpt_addr)
        print('i1 =', i1)
        self.assertEqual(o, True)
        print('loop1')
        self.BKPT_loop(bkpt_lb, i1)
        print('ignore')
        sdb.Wait_BKPT()
        self.assertTrue(sdb.queue.empty())
        self.assertEqual(sdb.Set_BKPT_Mode(i1, BKPT_Mode.Ignore), 0)
        sdb.Resume()
        time.sleep(2)
        self.assertTrue(sdb.queue.empty())
        print('loop2')
        sdb.Pause()
        self.assertEqual(sdb.Set_BKPT_Mode(i1, BKPT_Mode.BinCmd), 0)
        sdb.Resume()
        self.BKPT_loop(bkpt_lb, i1)
        print('clear')
        sdb.Wait_BKPT()
        self.assertEqual(sdb.Set_BKPT_Mode(0xff, BKPT_Mode.BPNone), 4)
        sdb.Resume()


if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyUSB0', 115200)
    queue = queue.Queue(1)
    mon = SerMon(ser, queue)
    sdb = SerDbg(ser, queue)
    mon.start()
    unittest.main()
