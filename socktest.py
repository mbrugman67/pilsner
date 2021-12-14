import socket
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', dest='host', required=True)
    parser.add_argument('--stuff', dest='stuff', required=True)
    args = parser.parse_args()

    print ('Sending packet to {}'.format(args.host))

    sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sck.settimeout(1.5)

    
    sck.sendto(bytearray(args.stuff, 'utf-8'),(args.host, 1234))
    data, addr = sck.recvfrom(1024)
    sck.close()
    print (data)
