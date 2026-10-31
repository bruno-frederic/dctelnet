"""A tiny SSH BBS that records the sessions test/test_ssh.c replays: one
connection, then exit. Needs paramiko (pip install paramiko).

  python3 tools/ssh_test_server.py 42211 curve password rekey &
  test/test_ssh record test/ssh_sessions/curve25519_password_rekey.bin 42211 guest sesame
  python3 tools/ssh_test_server.py 42212 group14 ki &
  test/test_ssh record test/ssh_sessions/group14_keyboard_interactive.bin 42212 guest sesame

usage: ssh_test_server.py PORT KEX(curve|group14) AUTH(password|ki) [rekey]"""
import os, socket, sys, threading, time
import paramiko

port, kexmode, authmode = int(sys.argv[1]), sys.argv[2], sys.argv[3]
rekey = "rekey" in sys.argv[4:]

keyfile = os.path.join(os.environ.get("TMPDIR", "/tmp"), "ssh_test_server_rsa.key")
if not os.path.exists(keyfile):
    paramiko.RSAKey.generate(2048).write_private_key_file(keyfile)
hostkey = paramiko.RSAKey(filename=keyfile)

class Server(paramiko.ServerInterface):
    def __init__(self):
        self.shell = threading.Event()
        self.size = None
        self.resized = threading.Event()
    def get_allowed_auths(self, user):
        return "password" if authmode == "password" else "keyboard-interactive"
    def check_auth_password(self, user, pw):
        return paramiko.AUTH_SUCCESSFUL if (user, pw) == ("guest", "sesame") else paramiko.AUTH_FAILED
    def check_auth_interactive(self, user, submethods):
        q = paramiko.InteractiveQuery("BBS login", "Answer two questions")
        q.add_prompt("Password: ", False)
        q.add_prompt("Favourite colour? ", True)
        return q
    def check_auth_interactive_response(self, responses):
        return paramiko.AUTH_SUCCESSFUL if list(responses) == ["sesame", "blue"] else paramiko.AUTH_FAILED
    def check_channel_request(self, kind, chanid):
        return paramiko.OPEN_SUCCEEDED if kind == "session" else paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED
    def check_channel_pty_request(self, ch, term, w, h, pw, ph, modes):
        self.size = (term, w, h)
        return True
    def check_channel_shell_request(self, ch):
        self.shell.set()
        return True
    def check_channel_window_change_request(self, ch, w, h, pw, ph):
        self.size = (self.size[0], w, h)
        self.resized.set()
        return True

ls = socket.socket(); ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
ls.bind(("127.0.0.1", port)); ls.listen(1)
print("listening", flush=True)
conn, _ = ls.accept()
t = paramiko.Transport(conn)
t.add_server_key(hostkey)
o = t.get_security_options()
o.kex = ("curve25519-sha256@libssh.org",) if kexmode == "curve" else ("diffie-hellman-group14-sha256",)
o.key_types = ("rsa-sha2-256",)
o.ciphers = ("aes128-ctr",)
o.digests = ("hmac-sha2-256",)
t.set_banner = None
srv = Server()
t.start_server(server=srv)
ch = t.accept(20)
srv.shell.wait(10)
if rekey:
    t.renegotiate_keys()
term, w, h = srv.size
ch.sendall("Welcome to the test BBS\r\nterm=%s cols=%d rows=%d\r\n> " % (term.decode() if isinstance(term, bytes) else term, w, h))
buf = b""
while b"\r" not in buf:
    buf += ch.recv(100)
ch.sendall(b"you said: " + buf.strip() + b"\r\n" + b"x" * 40000 + b"\r\n> ")
srv.resized.wait(10)
buf = b""
while b"\r" not in buf:
    buf += ch.recv(100)
ch.sendall("window %dx%d\r\n%s\r\n" % (srv.size[1], srv.size[2], buf.strip().decode()))
ch.send_exit_status(0)
ch.close()
time.sleep(0.5)
t.close()
print("done", flush=True)
