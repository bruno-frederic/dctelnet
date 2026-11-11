import socket, sys, time
art = open(sys.argv[1], 'rb').read()
art = art[:art.index(b'\x1a')]
art = art.replace(b'\xff', b'\xff\xff')
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('0.0.0.0', 2323)); s.listen(1)
c, a = s.accept(); print('connected', a, flush=True)
c.sendall(bytes([255, 251, 3, 255, 251, 1]))   # WILL SGA, WILL ECHO
time.sleep(2)
for i in range(0, len(art), 512):
    c.sendall(art[i:i+512]); time.sleep(0.05)
print('sent', len(art), flush=True)
time.sleep(float(sys.argv[2]) if len(sys.argv) > 2 else 90)
