########################################################
# pull.py
########################################################
# Network interface to the pilsner controller.  All
# comms are on a simple UDP socket.  Connect to UDP
# port 1234 and send a single character to do stuff:
#
#  'x' - this will request the contents of the logger,
#        pilz will return a long string of log data
#  'r' - reboot into UF2 bootloader mode
#  'n' - simple application reboot
########################################################

import socket
import argparse
import time

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', dest='host', required=True, help='Set the IP address')
    parser.add_argument('--logger', dest='logger', required=False, default=False, action='store_true', help='Pull log')
    parser.add_argument('--rebooten', dest='rebooten', required=False, default=False, action='store_true', help='Rebooten now!')
    parser.add_argument('--bootloader', dest='bootloader', required=False, default=False, action='store_true', help='Rebooten to bootloader')
    args = parser.parse_args()

    sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sck.settimeout(0.5)

    if args.rebooten == True:
        sck.sendto(bytearray('n', 'utf-8'), (args.host, 1234))
        print('Sending rebooten request!')
    elif args.bootloader == True:
        sck.sendto(bytearray('r', 'utf-8'), (args.host, 1234))
        print('Sending rebooten-to-bootloader request!')
    elif args.logger == True:
        print('Continual log pull:')
        while (1 == 1):
            try:
                sck.sendto(bytearray("x", 'utf-8'),(args.host, 1234))
                data, addr = sck.recvfrom(2048)
                print (data.decode(), end='')
                time.sleep(0.5)

            except KeyboardInterrupt:
                print('\nExiting')
                exit()

            except Exception as err:
                pass
    else:
        print('Pick a mode!')
