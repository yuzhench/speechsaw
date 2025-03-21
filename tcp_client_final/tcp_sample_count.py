import socket
import struct
import numpy as np
import time

SERVER_IP = "192.168.8.154"
SERVER_PORT = 8888
EXPECTED_SIZE = 1920+2+4  # You should set this to the expected size of your data

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((SERVER_IP, SERVER_PORT))
    server_socket.listen(1)
    print(f"Server is listening on {SERVER_IP}:{SERVER_PORT}")
    client_socket, client_address = server_socket.accept()

    signal = []  # preallocate numpy array
    np_values = np.zeros(EXPECTED_SIZE*2,dtype=np.uint8)
    idx = 0  # index for numpy array
    bytes_received = 0
    start_time = time.time()
    batery_start_time = time.time()

    while True:
        
        # print(f"Connection from {client_address}")
        message = client_socket.recv(2)
        temp = np.frombuffer(message, dtype=np.uint8)
        #messages_list.append(temp)
        signal = np.hstack((signal, temp))
        bytes_received += temp.shape[0]
        # print(bytes_received)
        elapsed_time = time.time() - start_time
        batery_time = (time.time() - batery_start_time) /60
        if elapsed_time > 1.0:  # 1 second
            print(f'Received {bytes_received} bytes in {elapsed_time} seconds.')
            print(f'battery time: {batery_time} minutes.')

            bytes_received = 0
            start_time = time.time()

        signal = np.asarray(signal, dtype='<B').view(np.uint16)
        hex_func = np.vectorize(hex)
        signal_hex = hex_func(signal)
        signal = []

        # print("start:",signal_hex[:10])
        # print("end:",signal_hex[-10:])

    client_socket.close()

if __name__ == "__main__":
    main()