#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#include "base64.h"

#define BUFSIZE 513
#define AUTHSOCKET "/var/run/dovecot/auth-client"

int connect_sock(struct sockaddr_un *sock_addr)
{
  int fd = -1;

  if (0 > (fd = socket(AF_UNIX, SOCK_STREAM, 0))) {
    return -1;
  }
  if (0 > connect(fd, (const struct sockaddr *) sock_addr,
      SUN_LEN(sock_addr))) {
    close(fd);
    return -1;
  }
  return fd;
}

int ask_dovecot(const char *credentials, long credentialslen)
{
  struct sockaddr_un dovecot_addr;
  char sockbuf[BUFSIZE] = { 0 };

  int dovesock, readn;

  if (0 >= credentialslen || !credentials)
    return 111;

  memset(&dovecot_addr, 0, sizeof(dovecot_addr));
  dovecot_addr.sun_family = AF_UNIX;

  strncpy(dovecot_addr.sun_path, AUTHSOCKET, sizeof(dovecot_addr.sun_path));

  if (0 > (dovesock = connect_sock(&dovecot_addr)))
    return 111;

  readn = read(dovesock, sockbuf, BUFSIZE - 1);
  if ((11 > readn) || (memcmp(sockbuf + readn - 5, "DONE\n", 5))) {
    close(dovesock);
    return 111;
  }

  sprintf(sockbuf,
    "VERSION\t1\t1\nCPID\t%i\nAUTH\t%i\tPLAIN\tservice=imap\tsecured\tresp=",
    getpid(), getpid() + 1);

  write(dovesock, sockbuf, strlen(sockbuf));
  write(dovesock, credentials, credentialslen);
  write(dovesock, "\n", 1);
  readn = read(dovesock, sockbuf, BUFSIZE - 1);
  close(dovesock);

  if ((3 < readn) && (!memcmp(sockbuf, "OK", 2)))
    return 0;

  return 1;
}

/* http://cr.yp.to/checkpwd/interface.html
 * ...not 100% conform to the interface
 *
 * http://wiki2.dovecot.org/Design/AuthProtocol
 */
int main(int argc, char **argv)
{
/* 'username\0password\0  on fd 3 */
  char buf[BUFSIZE] = { 0 };
  char dovebuf[BUFSIZE];

  int ret = 0;
  unsigned int buflen;
  char *login, *password;
  char *b64credentials;
  long b64len = 0;

  for (buflen = 0; buflen < sizeof(buf); buflen += ret) {
    do {
      ret = read(3, buf + buflen, sizeof(buf) - buflen);
    } while ((-1 == ret) && (errno == EINTR));
    if (-1 == ret)
      return (111);
    if (0 == ret)
      break;
  }
  close(3);

  login = buf;

  if (!(password = memchr(login, '\0', buflen)))
    return 2;
  ++password;
/* check if there is a password in blind mem */
  if (!memchr(password, '\0', buflen - (password - buf)))
    return 2;

/* '\0username\0password\0' */
  memset(dovebuf, 0, BUFSIZE);
  memcpy(dovebuf + 1, login, strlen(login));
  memcpy(dovebuf + 1 + strlen(login) + 1, password, strlen(password));

  b64credentials = malloc((strlen(login) + strlen(password) + 2) * 4 / 3 + 1);
  b64len =
    base64_encode(b64credentials, dovebuf,
    strlen(password) + strlen(login) + 3);
  ret = ask_dovecot(b64credentials, b64len);

  _exit(ret);
}
