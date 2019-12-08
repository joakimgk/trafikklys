
import sys
import socket

print("Socket create")

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port (IP is my PC)
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
	
    program = [[
    0b10000000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000
    ],
    [
    0b10000000,
    0b11000000,
    0b11100000,
    0b11000000
    ],
    [
    0b10100000,
    0b01000000,
    0b10100000
    ]] 

    try:

        #print >>sys.stderr, 'connection from', client_address
        print ("Connection from ", client_address)
        # Receive the data in small chunks and retransmit it
			
        while True:

            g = input("Tast 1-3 for å sende nytt program... ")
            i = int(g) - 1
            prog = program[i]
            
            Pakke = [3, len(program[i])] + program[i]
            connection.sendall(bytes(Pakke))
			
            g = input("Tast for å sende nytt RESET... ")
            Pakke = [4]
            connection.sendall(bytes(Pakke))
			
			
	     	
            g = input("Tast tempo: ");
            data = [1, 1, int(g)]
            Pakke = bytes(data)
            #Pakke = b'\x01\x01' + bytes(g, encoding='utf8')   #\x32'  # cmd = 1 | len = 1 | payload = 50 (0x32)
            #print (Pakke)
            connection.sendall(Pakke)
			
            #Pakke = '\x01\x01' + g;
			#connection.sendall(Pakke.encode())
            print("Tempo sendt")
	     	
            #Pakke = '\x02\x01\x32'  # cmd = 2 | len = 1 
            #input('Tast for å sende RESET... ')      # If you use Python 3
            #connection.sendall(Pakke.encode())
            #print("Reset sendt")
	        

    finally:
        print('error!!')
        # Clean up the connection
        connection.close()


