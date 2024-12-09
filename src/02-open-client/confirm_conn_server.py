#!/usr/bin/env python

import socket
import sys

if (len(sys.argv) != 2):
    print("usage: <program> port\n")
    exit(1)

HOST = 'localhost'
PORT = int(sys.argv[1])

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)

conn, addr = s.accept()

print("Connected by", addr)

conn.close()
