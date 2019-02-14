/*
 serialbridge - build bridges between serial ports and TCP sockets
 Copyright (c) 2013 Samuel Tardieu <sam@rfc1149.net>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <linux/termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "check.h"
#include "speed.h"
#include "serialconfig.h"
// #include "serialport.h"
#include "mbcrc.h"

extern void cfmakeraw(struct termios *);

char* ttymode;

static int verbose = 0;

// static void makeraw(int fd) {
//   struct termios t;
//   check(ioctl(fd, TCGETS, &t), "TCGETS");
//   cfmakeraw(&t);
//   check(ioctl(fd, TCSETS, &t), "TCSETS");
// }

static int serialsetup(const char *name, int speed, char *cflag) {
  ttymode = cflag;
//   printf("%s, %d, %s\n", name, speed, ttymode);
  int fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  ttydata_t mod;
  check(fd < 0, "open");
  mod.fd = fd;
  mod.port = (char *)malloc(sizeof name);
//   mod.port = &name;
  strcpy(mod.port, name);
  mod.speed = speed;
//   setspeed(fd, speed);
//   serialset(fd, speed, cflag);
//   printf("mod: %d, %s, %d\n", mod.fd, mod.port, mod.speed);
  tty_set_attr(&mod);
//   makeraw(fd);
//   printf("Serial port %s opend.\n", name);
  return fd;
}

// static int socketsetup(int port) {
//   int listening = socket(AF_INET6, SOCK_STREAM, 0);
//   check(listening < 0, "socket");
//   struct sockaddr_in6 addr;
//   memset(&addr, 0, sizeof addr);
//   addr.sin6_family = AF_INET6;
//   addr.sin6_port = htons(port);
//   int one = 1;
//   check(setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one), "SO_REUSEADDR");
//   check(bind(listening, (const struct sockaddr *)&addr, sizeof addr), "bind");
//   check(listen(listening, 5), "listen");
//   return listening;
// }

static void dump16(char prefix, const char *buffer, int length) {
  putchar(prefix);
  for (int i = 0; i < length; i++)
    printf(" %02x", (uint8_t) buffer[i]);
  for (int i = length; i < 16; i++)
    printf("   ");
  printf(" %c ", prefix);
  for (int i = 0; i < length; i++) {
    uint8_t c = buffer[i];
    putchar(c >= 32 && c <= 127 ? c : '.');
  }
  putchar('\n');
}

static void dump(char prefix, const char *buffer, int length) {
  for (; length >= 0; length -= 16, buffer += 16)
    dump16(prefix, buffer, length > 16 ? 16 : length);
}

static int relay(int from, int to, char prefix) {
    unsigned short crc = 0;
    unsigned short pdu_len = 0;
    static short tid = 0;
    static char buffer[1024];
    static char tpl[6]; //TID 2bytes, PID 2bytes, LENGTH 2bytes
    static char tmpbuf[1024];
    int n = read(from, buffer, sizeof buffer);
    // printf("Received: %d bytes.\n", n);
    // dump('<', buffer, n);
    if (n > 0) {
        if(prefix == '<')
        {
            crc = usMBCRC16(buffer, n - 2);
            if(crc == buffer[n-2] + buffer[n-1] * 256)
            {
                pdu_len = n - 2;
                memset(tmpbuf, 0, sizeof tmpbuf);
                memcpy(tmpbuf, tpl, 6);
                memcpy(tmpbuf + 6, buffer, pdu_len);
                tmpbuf[0] = tid >> 8;
                tmpbuf[1] = tid++ & 0xFF;
                tmpbuf[4] = pdu_len >> 8;
                tmpbuf[5] = pdu_len & 0xFF;
                write(to, tmpbuf, pdu_len + 6);
                //dump('*', tmpbuf, pdu_len + 6);
            }else
                perror("Wrong crc code");
        }else
        if(prefix == '>' && n >= 10) // from socket
        {
            crc = usMBCRC16(buffer + 6, n - 6);
            buffer[n++] = crc & 0xFF;
            buffer[n++] = crc >> 8;
            memcpy(tpl, buffer, 6);
            write(to, buffer + 6, n - 6);
        }
        //write(to, buffer, n);
        if (verbose)
            dump(prefix, buffer, n);
    }
    return n;
}

static void bridge(int sockfd, int serfd) {
  struct pollfd fds[] = {
    { serfd, POLLIN, 0 },
    { sockfd, POLLIN, 0 }
  };
  for (;;) {
    poll(fds, 2, -1);
    if (fds[0].revents & POLLIN)
      if (relay(serfd, sockfd, '<') <= 0) {
        fprintf(stderr, "Serial port closed\n");
        exit(0);
      }
    if (fds[1].revents & POLLIN)
      if (relay(sockfd, serfd, '>') <= 0)
	      return;
  }
}

static int ipverify(char *addr){
    char ipaddr[4][4];
    int i = 0;
    if(strlen(addr) < 7 || strlen(addr) > 15)
        return 0;
    sscanf(addr, "%[^.].%[^.].%[^.].%s", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
    for(i=0; i<4; i++)
        if(atoi(ipaddr[i])<0 || atoi(ipaddr[i])>255)
            return 0;
    return 1;
}

__attribute__((noreturn))
static void usage(int error, const char *progname) {
  fprintf(error ? stderr : stdout, "Usage: %s device speed cflag address port\neg: %s /dev/ttyS0 9600 8n1 127.0.0.1 502\n", progname, progname);
  exit(error);
}

int main(int argc, char * const argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hv")) != -1)
        switch(opt) {
        case 'v':
            verbose = 1;
            break;
        case 'h':
            usage(0, argv[0]);
        default:
            usage(1, argv[0]);
        }
    if (argc - optind != 5)
        usage(1, argv[0]);
    int speed = atoi(argv[optind + 1]);
    char *cflag = argv[optind + 2];
    char *addr = argv[optind + 3];
    int port = atoi(argv[optind + 4]);
    if (speed == 0 || port == 0 || ipverify(addr) == 0)
        usage(1, argv[0]);
    int serial = serialsetup(argv[optind], speed, cflag);
    //int listening = socketsetup(port);
    struct sockaddr_in serv;
    int sfd, ct;
    for (;;) {
        if (verbose)
            printf("Connecting to Server port.\n");
        sfd = socket(AF_INET,SOCK_STREAM, 0);//创建一个通讯端点
        if(sfd>=0){
            //初始化结构体serv
            serv.sin_family=AF_INET;
            serv.sin_port=htons(port);
            inet_pton(AF_INET, addr, &serv.sin_addr);
            ct = connect(sfd, (struct sockaddr *)&serv, sizeof(serv));
            if(ct>=0){
                if (verbose)
                    printf("Connection accepted\n");
                bridge(sfd, serial);
                close(sfd);
                if (verbose)
                    printf("Connection closed\n");
            }
            else
                perror("Socket connect error");
        }
        else //(sfd==-1)
            perror("Open socket error");
    }
    exit(0);
}
