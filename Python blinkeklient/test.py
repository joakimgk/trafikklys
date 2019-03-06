import sys
import socket
import time

print("Hei verden!")

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.connect
print("yes")


# Bind the socket to the port
server_address = ('192.168.43.254', 10000)
#print >>sys.stderr, 'starting up on %s port %s' % server_address
print("Starter server på port")
print(server_address)

sock.bind(server_address)

time.sleep(2)

# Listen for incoming connections
sock.listen(1)

connection, client_address = sock.accept()

print("OK")
