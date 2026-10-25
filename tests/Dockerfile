# Docker image for a small test server.
# Supports Telnet, Rlogin, SSH, and a Raw Connection shell (on TCP port 4444).
# Includes full ZMODEM file-transfer support via lrzsz.
#
# Build the image:
#   sudo docker build -t zmodem-server .
#
# Run the container with interactive debug output:
#   sudo docker run -ti --init --cap-add AUDIT_WRITE -p22:22 -p23:23 -p513:513 -p4444:4444 zmodem-server
#
# Important notes:
# - Run this container only on a trusted network isolated from the public internet by a firewall.
# - On Ubuntu, install buildx before building the image: sudo apt install docker-buildx
# - Save this Dockerfile with LF line endings; CRLF can cause xinetd configuration parsing errors.
#
# To debug SSH connections, append this command after the image name in docker run:
#      /usr/sbin/sshd -D -e -o PasswordAuthentication=yes -o LogLevel=DEBUG3
#   Requires the docker run option --init for Ctrl+C to work.

FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y \
        lrzsz socat xinetd inetutils-telnetd openssh-server rsh-server && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* && \
    mkdir /run/sshd     # needed by sshd on Debian bookworm

EXPOSE 22/tcp 23/tcp 513/tcp 4444/tcp

RUN cat << 'EOF' > /etc/motd
[1;36m+-----------------------------------------------------------+[0m
[1;36m|[0m   [1;32mWelcome on test server with ZMODEM[0m                      [1;36m|[0m
[1;36m|[0m   Type "download" or "upload" for file Zmodem transfer"   [1;36m|[0m
[1;36m+-----------------------------------------------------------+[0m
EOF

# https://manpages.debian.org/bookworm/passwd/useradd.8.en.html
RUN useradd --create-home --shell /bin/bash user  &&  echo "user:***" | chpasswd && \
    cat << 'EOF' > /home/user/.profile
alias ll='ls -alF'
alias download='sz'
alias upload='rz'
EOF

# Container entry point: run xinetd in foreground with debugging output:
CMD ["/usr/sbin/xinetd", "-dontfork", "-d"]

# Configure Telnet, raw TCP shell, Rlogin, and SSH as services under xinetd.
RUN cat << 'EOF' >> /etc/xinetd.conf
# TelnetD: https://manpages.debian.org/bookworm/inetutils-telnetd/telnetd.8.en.html
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

# Raw Connection: create a socat xinetd service for a raw TCP shell on port 4444
# https://manpages.debian.org/bookworm/socat/socat1.1.en.html
#
# Note: socat could also be used standalone in a container:
#   CMD socat TCP-LISTEN:4444,reuseaddr,fork EXEC:"/bin/login",pty,stderr
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

# RloginD: https://manpages.debian.org/bookworm/rsh-redone-server/rlogind.8
#
# The rlogind implementation only accpet client with a TCP source port in the range 512-1023.
# socat will relay connections from public TCP port 513 to rlogind on 127.0.0.1:1513, using the
# reserved source port 777.
# This hides the original client source port and limits the relay to 1 active connection at a time.
#
# Use only on a trusted test network; do not enable host-based trust for loopback.
service login
{
    disable     = no
    socket_type = stream
    wait        = no
    user        = root
    server      = /usr/bin/socat
    server_args = STDIO TCP:127.0.0.1:1513,sourceport=777,reuseaddr
    log_on_failure += USERID
}

service rlogin-internal
{
    disable     = no
    type        = UNLISTED
    socket_type = stream
    protocol    = tcp
    port        = 1513
    bind        = 127.0.0.1
    only_from   = 127.0.0.1
    wait        = no
    user        = root
    server      = /usr/sbin/in.rlogind
    log_on_failure += USERID
}

# sshd : https://manpages.debian.org/bookworm/openssh-server/sshd.8.en.html
#
# OpenSSH as an xinetd service handles one incoming connection per xinetd child process.
#   -i: Specifies that sshd is being run from inetd
#   PasswordAuthentication: enabled for this isolated test server.
#
# sshd requires docker run option: --cap-add AUDIT_WRITE
service ssh
{
    disable     = no
    socket_type = stream
    wait        = no
    user        = root
    server      = /usr/sbin/sshd
    server_args = -i -o PasswordAuthentication=yes
    log_on_failure += USERID
}
EOF
