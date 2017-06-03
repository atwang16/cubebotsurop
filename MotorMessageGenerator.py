# Motor Message Generator
# DRL Cubebots Project
# Spring 2017
# Miles Dai, Austin Wang

# Purpose
# Messages are sent through the access point (AP) to each end device (ED) via the SimpliciTI package
# provided by TI.

# The target boards being used do not have enough memory to store the device addresses of all the end points
# within a single access point. The solution is to send messages through the access point where they will be 
# broadcast to all of the EDs simultaneously. 

# Each ED only stores its own device address and scans the message to see if there is one for it. Therefore, each 
# message that is sent out will include a two-byte device address that precedes a 6-byte motor message. Two bytes 
# are required at the front of the message to indicate to the EDs that the messsage is a motor as opposed to a time 
# message which the AP will handle simultaneously. The AP can only transmit messages for up to 6 motors at a time. 
# Below is the anatomy of a motor message

# 			Broadcast message diagram
# 			+------+--+------+--+------+--+------+--+------+--+------+--+--+
# 			|  6   |2 |  6   |2 |  6   |2 |  6   |2 |  6   |2 |  6   |2 |2 |   Tx
# 			|      |  |      |  |      |  |      |  |      |  |      |  |  | ------>
# 			+------+--+------+--+------+--+------+--+------+--+------+--+--+
# Bytes -->	49                                                 9    4  2 1 0

# Each broadcast has data for 6 motors.
# Each block of 6 bytes is instructions for a motor.
# Each block of 2 bytes is the ED address (last two bytes of lAddr).
# The right-most block of 2 bytes is the flag indicating this is a motor message (MOT_MSG).

import serial

# Do not change this. This is the code used by the ED to differentiate between
# time and motor messages. This code is defined in smpl_nwk_config.dat
MOT_MSG = [0xFF, 0xFF]

# Device addresses of each motor. Must match the last two digits of THIS_DEVICE_ADDRESS in the "smpl_config_ed.dat" file.
MOTORS = {'a': (0x00, 0x01), 'b': (0x00, 0x02), 'c': (0x00, 0x03)}

# Desired parameters for each motor (amp, freq, phaseShift)
MOTOR_PARAM = {'a': (1,0.5,0), 'b': (0,0,0), 'c': (0,0,0)}


def generate_realTerm_message():
    ''' Outputs a file that contains hex code that can be imported into RealTerm
    '''
    global MOT_MSG, MOTORS, MOTOR_PARAM
    
    FINAL_MSG = ['$' + hex(x)[2:] + ' ' for x in MOT_MSG]
    
    
    for motor in iter(MOTORS.keys()):
        # Add motor address
        FINAL_MSG.extend(['$' + hex(x)[2:].zfill(2) + ' ' for x in MOTORS[motor]])
        
        # Add scaled parameters
		# TODO: the final message may not need to contain the reversed hex bytes. Ran out of time to confirm on the actual cubebot
		# If no reversal is necessary, reverse the order of the two appends and change the slicing within the processing of each parameter
        amp = int(MOTOR_PARAM[motor][0] * 500) # Scale up by a factor of 500
        FINAL_MSG.append('$' + hex(amp % 256)[-1:1:-1].zfill(2) + ' ') # Generate last two digits of hex byte and reverse them
        FINAL_MSG.append('$' + hex(amp // 256)[-1:1:-1].zfill(2) + ' ')

        
        freq = int(MOTOR_PARAM[motor][1] * 500)
        FINAL_MSG.append('$' + hex(freq % 256)[-1:1:-1].zfill(2) + ' ')
        FINAL_MSG.append('$' + hex(freq // 256)[-1:1:-1].zfill(2) + ' ')
        
        phaseShift = int(MOTOR_PARAM[motor][2] * 500)
        FINAL_MSG.append('$' + hex(phaseShift % 256)[-1:1:-1].zfill(2) + ' ')
        FINAL_MSG.append('$' + hex(phaseShift // 256)[-1:1:-1].zfill(2) + ' ')
 
    FINAL_MSG = ''.join(FINAL_MSG)
    
    print(FINAL_MSG)
    # TODO: output to a text file instead of printing

def write_to_serial():
    ''' Generates a raw hexcode motor message that is output directly to the
    serial port with the MSP430 wireless dev tool (USB hub) in it

    TODO: This function is unfinished. The AP is receiving the message as indicated by the
    green LED toggling, but the EDs are not recieving messages.
    '''
    
    global MOT_MSG, MOTORS, MOTOR_PARAM
    
    final_msg = MOT_MSG.copy()
    
    for motor in iter(MOTORS.keys()):
        # Add motor address
        final_msg.extend(MOTORS[motor])
        
        # Add scaled parameters
        amp = int(MOTOR_PARAM[motor][0] * 500)
        final_msg.append(amp // 256)
        final_msg.append(amp % 256)
        
        freq = int(MOTOR_PARAM[motor][1] * 500)
        final_msg.append(freq // 256)
        final_msg.append(freq % 256)
        
        phaseShift = int(MOTOR_PARAM[motor][1] * 500)
        final_msg.append(phaseShift // 256)
        final_msg.append(phaseShift % 256)
    
    # print(final_msg)
    final_msg = bytearray(final_msg)
    # print(final_msg)

    # Writing to serial port
	
	# Adjust the port to the one you're using
    port = 'COM10'
    baud_rate = 57600
    timeout = 5
    write_timeout = 5
    
    # Close out of any software that might be accessing serial ports (e.g. Realterm)
    ser = serial.Serial(port, baud_rate, timeout = timeout, write_timeout = write_timeout)
    # First make sure port is closed to prevent exception ...
    ser.close()
    # ... then reopen
    ser.open()
    print('------------------------\nWriting {} to {}\n'.format(final_msg, port))
    ser.write(final_msg)
    print('Success!\n\nWaiting for return message...\n')
    print(ser.read(len(final_msg)))
    print('\nFinished. Closing port.')
    ser.close()
    
# write_to_serial()
generate_realTerm_message()