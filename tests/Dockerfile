# Docker image providing a standard Telnet server and a raw TCP shell service on port 4444, with
# full ZMODEM file transfer support (via the lrzsz package).
#
# For raw TCP access, make sure to select the “✔ Raw Connection” option in DCTelnet.
#
# Build the image:
#   sudo docker build -t zmodem-server .
#
# Run the container with interactive debug output:
#   sudo docker run -ti -p 23:23 -p 4444:4444 zmodem-server
#
# Once connected, you can use sz/rz to send and receive files using ZMODEM.

FROM debian:stable-slim

RUN apt-get update && \
    apt-get install -y lrzsz socat xinetd inetutils-telnetd  && \
    apt-get clean

RUN cat << 'EOF' > /etc/motd
[1;36m+-----------------------------------------------------------+[0m
[1;36m|[0m   [1;32mWelcome on test server with ZMODEM[0m                      [1;36m|[0m
[1;36m|[0m   Type "download" or "upload" for file Zmodem transfer"   [1;36m|[0m
[1;36m+-----------------------------------------------------------+[0m
EOF

# https://manpages.debian.org/latest/passwd/useradd.8.html
RUN useradd --create-home --shell /bin/bash user  &&  echo "user:***" | chpasswd


RUN cat << 'EOF' > /home/user/.profile
alias ll='ls -alF'
alias download='sz'
alias upload='rz'
EOF

# https://manpages.debian.org/latest/inetutils-telnetd/telnetd.8.en.html
EXPOSE 23
RUN cat << 'EOF' > /etc/xinetd.d/telnet
service telnet
{
    disable     = no
    flags       = REUSE
    socket_type = stream
    wait        = no
    user        = root
    server      = /usr/sbin/telnetd
    server_args = --debug --no-hostinfo --exec-login /bin/login
    log_on_failure += USERID
}
EOF

# Create a socat xinetd service for a raw TCP shell on port 4444
#
# Note: socat could also be used standalone in a container:
#   CMD socat TCP-LISTEN:4444,reuseaddr,fork EXEC:"/bin/login",pty,stderr
EXPOSE 4444
RUN cat << 'EOF' > /etc/xinetd.d/rawtcp
service rawtcp
{
    disable     = no
    socket_type = stream
    protocol    = tcp
    port        = 4444
    type        = UNLISTED
    wait        = no
    user        = root
    server      = /usr/bin/socat
    server_args = STDIO EXEC:/bin/login,pty,stderr
    log_on_failure += USERID
}
EOF

# Start xinetd in foreground with debugging output:
ENTRYPOINT ["xinetd"]
CMD ["-dontfork", "-d"]
