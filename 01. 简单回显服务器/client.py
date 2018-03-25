#!/usr/bin/env python
#!encoding: utf-8

import socket

conn_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
conn_fd.connect(('127.0.0.1', 7777))

msg = conn_fd.recv(1024)
print(msg)

while True:
    data = raw_input('发送: ')
    conn_fd.send(data)
    if data == 'exit':
        conn_fd.close()
        break
    else:
        msg = conn_fd.recv(1024)
        print('收到: ' + msg)
