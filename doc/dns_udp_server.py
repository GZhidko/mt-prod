import socket

# Set up UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.bind(('18.0.0.1', 53))  # Replace with the port you want to listen on

while True:
    # Receive data
    data, addr = udp_socket.recvfrom(1024)
    print("Received data:", data.decode(), "from", addr)

    # Process received data and prepare response
    # For simplicity, let's just send back the same data
    response_data = data

    # Send response back to the client
    udp_socket.sendto(response_data, addr)

