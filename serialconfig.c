/*
 * OpenMODBUS/TCP to RS-232/485 MODBUS RTU gateway
 *
 * tty.c - terminal I/O related procedures
 *
 * Copyright (c) 2002-2003, 2013, Victor Antonovich (v.antonovich@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: tty.c,v 1.4 2015/02/25 10:33:58 kapyar Exp $
 */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
//#include <poll.h>
//#include <stropts.h>
// #include <linux/termios.h>
#include <termios.h>
// #include <unistd.h>  // UNIX Standard Definitions

//#include "check.h"
#include "serialconfig.h"

extern char* ttymode;
// static int tty_break;

/*
 * Flag signal SIG
 */
// void
// tty_sighup(void)
// {
//   tty_break = TRUE;
//   return;
// }

/*
 * Init serial link parameters MOD to PORT name, SPEED and TRXCNTL type
 */
void
tty_init(ttydata_t *mod, char *port, int speed)
{
  mod->fd = -1;
  mod->port = port;
  mod->speed = speed;
}

/*
 * Opening serial link whith parameters in MOD
 */
int
tty_open(ttydata_t *mod)
{
  if (mod->fd > 0)
    return RC_AOPEN;        /* if already open... */
//   tty_break = FALSE;
  mod->fd = open(mod->port, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if (mod->fd < 0)
    return RC_ERR;          /* attempt failed */
  return tty_set_attr(mod);
}

/*
 * Setting up tty device MOD attributes
 */
int
tty_set_attr(ttydata_t *mod)
{
  int flag;
  speed_t tspeed = tty_transpeed(mod->speed);

  memset(&mod->savedtios, 0, sizeof(struct termios));
  if (tcgetattr(mod->fd, &mod->savedtios))
    return RC_ERR;
  memcpy(&mod->tios, &mod->savedtios, sizeof(mod->tios));
  mod->tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  mod->tios.c_oflag &= ~OPOST;
  mod->tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  mod->tios.c_cflag |= CREAD | CLOCAL ;
  mod->tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB | PARODD);
  switch (ttymode[0])
  {
    case '5':
      mod->tios.c_cflag |= CS5;
      break;
    case '6':
      mod->tios.c_cflag |= CS6;
      break;
    case '7':
      mod->tios.c_cflag |= CS7;
      break;
    case '8':
      mod->tios.c_cflag |= CS8;
      break;
  }
  switch (ttymode[1])
  {
    case 'E':
    case 'e':
      mod->tios.c_cflag |= PARENB;
      break;
    case 'O':
    case 'o':
      mod->tios.c_cflag |= (PARENB | PARODD);
      break;
  }
  if (ttymode[2] == '2')
  {
      mod->tios.c_cflag |= CSTOPB;
  }
  mod->tios.c_cc[VTIME] = 0;
  mod->tios.c_cc[VMIN] = 6;
#ifdef HAVE_CFSETSPEED
  cfsetspeed(&mod->tios, tspeed);
#else
  cfsetispeed(&mod->tios, tspeed);
  cfsetospeed(&mod->tios, tspeed);
#endif
  if (tcsetattr(mod->fd, TCSANOW, &mod->tios))
    return RC_ERR;
  tcflush(mod->fd, TCIOFLUSH);
  flag = fcntl(mod->fd, F_GETFL, 0);
  if (flag < 0)
    return RC_ERR;
  return fcntl(mod->fd, F_SETFL, flag | O_NONBLOCK);
}

/*
 * Translate integer SPEED value to speed_t constant
 */
speed_t
tty_transpeed(int speed)
{
  speed_t tspeed;
  switch (speed)
  {
  case 0:
    tspeed = B0;
    break;
#if defined(B50)
  case 50:
    tspeed = B50;
    break;
#endif
#if defined(B75)
  case 75:
    tspeed = B75;
    break;
#endif
#if defined(B110)
  case 110:
    tspeed = B110;
    break;
#endif
#if defined(B134)
  case 134:
    tspeed = B134;
    break;
#endif
#if defined(B150)
  case 150:
    tspeed = B150;
    break;
#endif
#if defined(B200)
  case 200:
    tspeed = B200;
    break;
#endif
#if defined(B300)
  case 300:
    tspeed = B300;
    break;
#endif
#if defined(B600)
  case 600:
    tspeed = B600;
    break;
#endif
#if defined(B1200)
  case 1200:
    tspeed = B1200;
    break;
#endif
#if defined(B1800)
  case 1800:
    tspeed = B1800;
    break;
#endif
#if defined(B2400)
  case 2400:
    tspeed = B2400;
    break;
#endif
#if defined(B4800)
  case 4800:
    tspeed = B4800;
    break;
#endif
#if defined(B7200)
  case 7200:
    tspeed = B7200;
    break;
#endif
#if defined(B9600)
  case 9600:
    tspeed = B9600;
    break;
#endif
#if defined(B12000)
  case 12000:
    tspeed = B12000;
    break;
#endif
#if defined(B14400)
  case 14400:
    tspeed = B14400;
    break;
#endif
#if defined(B19200)
  case 19200:
    tspeed = B19200;
    break;
#elif defined(EXTA)
  case 19200:
    tspeed = EXTA;
    break;
#endif
#if defined(B38400)
  case 38400:
    tspeed = B38400;
    break;
#elif defined(EXTB)
  case 38400:
    tspeed = EXTB;
    break;
#endif
#if defined(B57600)
  case 57600:
    tspeed = B57600;
    break;
#endif
#if defined(B115200)
  case 115200:
    tspeed = B115200;
    break;
#endif
  default:
    tspeed = DEFAULT_BSPEED;
    break;
  }
  return tspeed;
}

/*
 * Prepare tty device MOD to closing
 */
int
tty_cooked(ttydata_t *mod)
{
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  if (!isatty(mod->fd))
    return RC_ERR;
  if (tcsetattr(mod->fd, TCSAFLUSH, &mod->savedtios))
    return RC_ERR;
  return RC_OK;
}

/*
 * Closing tty device MOD
 */
int
tty_close(ttydata_t *mod)
{
  if (mod->fd < 0)
    return RC_ACLOSE;       /* already closed */
  if (tty_cooked(mod))
    return RC_ERR;
  return close(mod->fd);
}


