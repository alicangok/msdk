#! /usr/bin/env python3

###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 # (now owned by Analog Devices, Inc.),
 # Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 # is proprietary to Analog Devices, Inc. and its licensors.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################

## rf_switch.py
 #
 # Tool used to control an RF switch with a simple UART interface
 #

import serial
import sys
import argparse
from argparse import RawTextHelpFormatter
from time import sleep

# Setup the default serial port settings
defaultBaud=115200
defaultSP="/dev/ttyUSB0"

# Namespace class used to create function arguments similar to argparse
class Namespace:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

class rf_switch:

    port = serial.Serial()
    serialPort = ""

    def __init__(self, args):
        
        try:
            # Open serial port
            serialPort = args.serialPort
            self.port = serial.Serial(
                port=str(serialPort),
                baudrate=args.baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                rtscts=False,
                dsrdtr=False,
                timeout=1.0
            )
            self.port.isOpen()
        except serial.SerialException as err:
            print(err)
            sys.exit(1)

        except OverflowError as err:
            print("baud rate exception, "+str(args.baud)+" is too large")
            print(err)
            sys.exit(1)

        # Send the command, adding carriage return
        commandString = args.command+"\r"
        for char in commandString:
            b = bytearray()
            b.extend(map(ord, char))
            self.port.write(b)
            sleep(0.01)

        # Receive the status
        response = self.port.read_until("\n")
        print(response.decode())
        
if __name__ == '__main__':


    # Setup the command line description text
    descText = """
    RF Switch tool.

    This tool is used to control an RF switch. The switch has two sides (L and R), each with
    4 inputs. 

    Serial port is configured as 8N1, no flow control, default baud rate of """+str(defaultBaud)+""".
    """

    # Parse the command line arguments
    parser = argparse.ArgumentParser(description=descText, formatter_class=RawTextHelpFormatter)
    parser.add_argument('serialPort', default=defaultSP,
                        help='Serial port path or COM#, default: '+defaultSP)
    parser.add_argument('command', help="""Set the left or right switch: L or R, followed by 0-4. 0 indicating closed.
        Example: l1""")

    parser.add_argument('--baud', '-b', default=defaultBaud,
                        help='Serial port baud rate, default: '+str(defaultBaud))

    args = parser.parse_args()
    serialPort = args.serialPort
    print("RF Switch tool")
    print("Serial port: "+serialPort)
    print("8N1 "+str(args.baud))
    print("")

    # Run the command
    ble_hci = rf_switch(args)

