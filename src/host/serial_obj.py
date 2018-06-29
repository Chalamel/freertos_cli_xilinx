""" Class used to create a MainSerial object, which can be used to write- and read data from a given
serial port """

import serial

class MainSerial(object):
    """ Creates a MainSerial object with the given parameters """
    

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

        self.seq_num = 0

        if not self.ser.isOpen():
            raise RuntimeError("Could not open serial port. Exiting.")
        else:
            print("Serial configured and opened.")
