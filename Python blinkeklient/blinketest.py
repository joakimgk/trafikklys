
import sys
import socket

print("Socket create")

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('192.168.43.254', 10000)
#print >>sys.stderr, 'starting up on %s port %s' % server_address
print("Starter server på port")
print(server_address)
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print('waiting for a connection')

    connection, client_address = sock.accept()

    try:
        #print >>sys.stderr, 'connection from', client_address
        print ("Connection from ", client_address)
        # Receive the data in small chunks and retransmit it
        while True:
            data = connection.recv(256)
            #print >>sys.stderr, 'received "%s"' % data
            print("Mottatt data: ", data)
            if data:
                #print >>sys.stderr, 'sending data back to the client'
                print("Send data tilbake")
                connection.sendall(data)
            else:
                #print >>sys.stderr, 'no more data from', client_address
                print("Ingen flere data")
                break

    finally:
        # Clean up the connection
        connection.close()
