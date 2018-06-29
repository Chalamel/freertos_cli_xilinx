""" Simple serial client """

from serial_obj import MainSerial
import struct

gpio_base_addr = 0x41200000

class SerialClient(object):
    
    def __init__(self, serial_obj):
        if not isinstance(serial_obj, MainSerial):
            raise ValueError("serial_obj in SerialClient constructor should be of MainSerial type.")

        self.interface = serial_obj
    

    def write(self, addr, val):
        """ Writes the value "val" to the AXI-address "addr" """
        self.interface.ser.write(("wr "+str(addr)+" "+str(val)+'\n').encode())
    
    def read(self, addr):
        """ Reads the value stored at the AXI-address "addr" """
        self.interface.ser.write(("rd "+str(addr)+'\n').encode())
        return self.interface.ser.readline()[:-1]

    def get_serial_cmds(self, print_res=False):
        """ Returns and optionally prints a list of strings where each string
        describes a valid CLI command """
        ret = []

        self.interface.ser.write(("help"+'\n').encode())
        cmds = self.interface.ser.readlines()

        for desc in cmds:
            if print_res:
                print(desc.decode("utf-8"))
            ret.append(desc.decode("utf-8"))
    
        return ret


#Example usage. Port "COM6", baudrate 57600, no parity, 1 stopbit, 1 second receive-timeout
client = SerialClient(MainSerial("/COM6", 57600, "none", 1, 1))
client.get_serial_cmds(print_res=True)