/*
 * $Id: httpget.c $
 *
 * 	Copyright (c) 1996 Network Solutions Development Inc.
 * 	        All rights reserved
 *
 * Created: Thu Feb 15 22:12:54 1996 too
 * Last modified: Sat Mar  2 17:18:00 1996 too
 *
 * HISTORY 
 * $Log: $
 */

/*
 * To make sure no code can get run before `start'.
 */

int start(void);
int begin(void) { return start(); }

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/socket.h>
/*#include <proto/usergroup.h> */

struct ExecBase * SysBase;
struct DosLibrary * DOSBase;
struct Library * SocketBase;


/* general system / socket includes */
#include <sys/types.h>
#include <sys/socket.h>

/* internet socket stuff */
#include <netinet/in.h>

/* host name resolvation/database stuff */
#include <netdb.h>

/*
 *
 */

/* #define PutCS(s) Write(Output(), s, sizeof (s) - 1) */

#define TEMPLATE "URL/A"

/*
 * My 'startup code' initialize bases, read arguments and call `main'
 * httpget -routine.
 */
int httpget(char * url);

#ifdef __GNUC__
/* saveds is used in SASC to get global data base pointer. SAS/C compiles
   base relative code by default, which makes target binary somewhat smaller */
#define __saveds
#endif

int __saveds start()
{
  int rv = 20;	/* default exit value */
  SysBase = *(struct ExecBase **)4;

  if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37)) != NULL)
  {
    if ((SocketBase = OpenLibrary("bsdsocket.library", 4)) != NULL)
    {
      struct RDArgs * rdargs;
      char * Argv[1] = { 0 };
      
      if ((rdargs = ReadArgs(TEMPLATE, (LONG *)Argv, NULL)) != NULL)
      {
	rv = httpget(Argv[0]);
	FreeArgs(rdargs);
      }
      else
	Printf("Usage: httpget " TEMPLATE "\n");

      CloseLibrary(SocketBase);
    }
    else
      Printf("Cannot open bsdsocket.library\n");

    CloseLibrary((struct Library *)DOSBase);
  }
  return rv;
}

/*
 * Since this is a no-link-libraries -example, we have to write all the
 * most trivial functions ourselves. It would be wise to use standard
 * c-link-library for these, but just skip startup code.
 * (optimized library functions are the smallest and fastest possible)
 */

void bzero(char * p, int len)
{
  while (len--)
    *p++ = '\0';
}

int memcmp(char * s1, char * s2, int n)
{
  int c;

  while (n--)
    if ((c = (*s1++ - *s2++)) != 0)
      return c;

  return 0;
}

#if 1
#define memcpy(a,b,c) CopyMem((b),(a),(c))
#else
void memcpy(char * d, char * s, int l)
{
  while (l)
  {
    l--;
    *d++ = *s++;
  }
}
#endif

int strlen(char * str)
{
  int i = 0;

  while (*str) {
    str++;
    i++;
  }
  return i;
}  


char * strchr(char * string, char c)
{
  while (*string)
    if (*string == c)
      return string;
    else
      string++;

  return NULL;
}

char * strstr(char * string, char * s)
{
  while (*string)
    if (strchr(s, *string) != NULL)
      return string;
    else
      string++;
  
  return NULL;
}

int isdigit(char c)
{
  return (c >= '0' && c <= '9');
}

int atoi(char * str)
{
  int i = 0;

  while (isdigit(*str))
    i = i * 10 + *str++ - '0';

  return i;
}

/*
 * Let's define some useful routines for this application.
 */

static int printUrlFault(void)
{
  Printf("Url has to have format: [http://]host.domain.net[:port]/path\n");
  return 20;
}


/*
 * And finally to our `main' routines.
 */
static int	do_connect(char * name, int port);
static void	netperror(char * banner);

int httpget(char * url)
{
  char buf[1024]; /*stack usage of program should fit in 4000 bytes of memory*/
  char * p, * u;  /* ^^^ in current program 1k local buffer is sufficient */
  int i, port, sd;

  /*
   * A URL may start w/ http:// or without it in this application.
   */ 
  if (memcmp(url, "http://", 7) == 0)
    url+= 7;
  
  if ((p = strstr(url, ":/")) == NULL)
    return printUrlFault();

  if (*p == ':') {
    /*
     * user want's to access custom port/service.
     */
    u = strchr(p, '/');

    if (u == NULL)
      return printUrlFault();

    if (u == p) {
      Printf("No port/service defined\n");
      return 20;
    }
    /*
     * But now, I'm too lazy to write full `getservbyname' query here,
     * so only numeric port information will do
     */
    if ((port = atoi(p+1)) == 0) {
      Printf("Given port not numeric\n");
      return 20;
    }
  }
  else {
    u = p;
    port = 80;	/* standard http -service port number */
  }

  *p = '\0';

  if ((sd = do_connect(url, port)) < 0)
    return 20;

  *p = '/';
  /*
   * I could have done a hack to write "GET " at position u - 4 and done
   * a single send() call to peer. I don't know a hostname that fits in 3
   * bytes and get positive answer from name server. Anyway, that would
   * have been too `kludgy' way to solve the problem in this example.
   */

  i = strlen(u);
  u[i++] = '\n'; /* here it is safe to replace terminating NUL w/ '\n';
  
/*  Printf("Sending `%s'\n",u); */
  /*
   * unless fails, send will send all bytes, since in NONBLOCKing mode
   */
  if ((send(sd, "GET ", 4, 0) != 4) || (send(sd, u, i, 0) != i)) {
    netperror("send");
    return 20;
  }

  while ((i = recv(sd, buf, sizeof buf, 0)) > 0)
    Write(Output(), buf, i);

  if (i != 0) {
    netperror("recv");
    return 20;
  }
  
  return 0;
}

/*
 * Stuff to make connection. Reuse freely.
 */
int
getSockAddr(const char *	 host,
	    int			 port,
	    struct sockaddr_in * addr)
{
  struct hostent * remote;

  if ((remote = gethostbyname(host)) != NULL)
    memcpy(&addr->sin_addr, remote->h_addr, sizeof addr->sin_addr);
  else if ((addr->sin_addr.s_addr = inet_addr(host)) == (unsigned long)-1) {
    return FALSE;
  }

  addr->sin_port = htons(port);
  addr->sin_family = AF_INET;

  return TRUE;
}

const int ONE = 1;

int inetconn (const struct sockaddr_in * addr)
{
  int sd;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    netperror ("socket");
    return -1;
  }
  
  setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &ONE, sizeof ONE);
    
  if (connect(sd, (struct sockaddr *)addr, sizeof (struct sockaddr_in)) < 0) {
    CloseSocket(sd);
    netperror("connect");
    return -1;
  }

  return sd;
}


static int do_connect(char * host, int port)
{
  struct sockaddr_in
    addr = { 0 }; /* implicit bzero() here when compiled w/ gcc */
  
  if (getSockAddr(host, port, &addr) == FALSE) {
    Printf("Cannot resolve host address\n");
    return -1;
  }

  return inetconn(&addr);
}
  

/*
 * This `perror' works only for sockets. Therefore hidden here at the end
 * of this program. There is a general perror function available in network
 * link library without any additional speed/size overhead. This is used to
 * demonstrate the power of `SocketBaseTagList()' function.
 */

#include <amitcp/socketbasetags.h>

static void netperror(char * banner)
{
  ULONG taglist[5];
  
  taglist[0] = SBTM_GETVAL(SBTC_ERRNO);
  /* taglist[1] set when tag 0 executed */
  taglist[2] = SBTM_GETREF(SBTC_ERRNOSTRPTR);
  taglist[3] = (LONG)&taglist[1]; /* reads and writes tag[1] */
  taglist[4] = NULL;	/* TAG_END if <utility/tagitem> is included */

  SocketBaseTagList((struct TagItem *)taglist);

  if (banner)
    Printf("%s:", banner);

  Printf("%s\n", (char *)taglist[1]);
}
