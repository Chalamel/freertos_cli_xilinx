""" Simple communications client. Constructor takes an object "interface" that must provide the
methods:

write(self, param: str) - writes the parameter of to the interface
readline(self)          - returns a line from the interface receive-buffer. A "line" is simply some input followed by
                          the '\n' character 
readlines(self)         - returns all lines contained within the interface receive-buffer

An interface in this case may be any type of communication-interface, such as a UART, socket, etc
"""

from serial_obj import SerialObj
import struct


class ComsClient(object):
    
    def __init__(self, coms_if):
        self.interface = coms_if
    
    def write(self, addr, val):
        """ Writes the value "val" to the address "addr" """
        self.interface.write(("wr "+str(addr)+" "+str(val)+'\n').encode())
    
    def read(self, addr):
        """ Reads the value stored at the address "addr" """
        self.interface.ser.write(("rd "+str(addr)+'\n').encode())
        return self.interface.readline()

    def get_cmds(self, print_res=False):
        """ Returns and optionally prints a list of strings where each string
        describes a valid CLI command """
        ret = []

        self.interface.ser.write(("help"+'\n').encode())
        cmds = self.interface.readlines()

        for desc in cmds:
            if print_res:
                print(desc.decode("utf-8"))
            ret.append(desc.decode("utf-8"))
    
        return ret


#Example usage. Port "COM6", baudrate 57600, no parity, 1 stopbit, 1 second receive-timeout
serial_obj = SerialObj("/COM1", 57600, "none", 1, 1)
client = ComsClient(serial_obj)

client.get_cmds(print_res=True)
client.write(0xDEADBEEF, 0xCAFEBABE)