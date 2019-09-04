""" Class defines a serial interface. An object of this type can be used together with
a ComsClient instance and the "FreeRTOS + CLI for remote address-space access" CLI
"""

import serial

class SerialObj(object):
    """ Creates a SerialObj object with the given parameters """
    

    def __init__(self, port, baudrate, parity, stopbits, timeout):

        if parity.lower() == "none":
            parity = serial.PARITY_NONE
        elif parity.lower() == "even":
            parity = serial.PARITY_EVEN
        elif parity.lower() == "odd":
            parity = serial.PARITY_ODD
        else:
            raise ValueError("Parity must be \"none\", \"even\", or \"odd\"!")

        if stopbits == 1:
            stopbits = serial.STOPBITS_ONE
        elif stopbits == 2:
            stopbits = serial.STOPBITS_TWO
        else:
            raise ValueError("Stopbits must be either 1 or 2!")

        self.ser = serial.Serial(
            port=port, #'/COM1', 'dev/tty0' etc
            baudrate=baudrate,
            parity=parity,
            stopbits=stopbits,
            bytesize=serial.EIGHTBITS,
            timeout=timeout,
        )

        if not self.ser.isOpen():
            raise RuntimeError("Could not open serial port. Exiting.")
        else:
            print("Serial configured and opened.")

    def write(self, data: str):
        """ Write "data" using the I/F """
        self.ser.write(data)

    def readline(self):
        """ Return a line stored in the receive-buffer """
        return self.ser.readline()[:-1]
    
    def readlines(self):
        """ Return all lines stored in the receive-buffer """
        return self.ser.readlines()
