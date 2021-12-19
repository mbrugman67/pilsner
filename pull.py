import socket
import argparse
import time

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', dest='host', required=True)
    args = parser.parse_args()

    sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sck.settimeout(0.5)

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
