#!/usr/bin/env python

import socket
import sys

if (len(sys.argv) != 2):
    print(f"usage: {sys.argv[0] if len(sys.argv) > 1 else "<program>"} <port>\n")
    exit(1)

HOST = 'localhost'
PORT = int(sys.argv[1])

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect((HOST, PORT))
print("In client - Connected to server!")

s.close()
