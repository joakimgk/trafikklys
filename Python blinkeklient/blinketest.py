
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
		
		# vent på "KLAR FOR PROGRAM"
		# send tempo
		# (send nytt program)
		
        print ('OK')
		
        while True:
            print('kødd')
            data = connection.recv(256)
            #print >>sys.stderr, 'received "%s"' % data
            print("Mottatt data: ", data)
            if data:
                print ('Da kjører vi!')
                break;  # bryt ut av while True
		#
		
        input('Tast for å fortsette')      # If you use Python 3
		
        tempoSent = False
        programSent = False
		
        tempoPakke = '\x01\x01\x32'  # cmd = 1 | len = 1 | payload = 50 (0x32)
        while True:  # simulere UX -- skal ikke motta noe
		    # send tempo
            if not tempoSent:
                connection.sendall(tempoPakke.encode())
                tempoSent = True
                print("Tempo sendt...")

    finally:
        print('error!!')
        # Clean up the connection
        connection.close()
