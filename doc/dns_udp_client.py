import socket

# Server address and port
server_address = ('18.0.0.1', 53)

# Message to send
message = b"Hello, server!"

# Create UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind socket to specific source IP address (10.0.0.5)
udp_socket.bind(('10.0.0.5', 0))

# Send message to server
udp_socket.sendto(message, server_address)

# Receive response from server
response, server = udp_socket.recvfrom(1024)
print("Received response:", response.decode(), "from", server)

# Close socket
udp_socket.close()

