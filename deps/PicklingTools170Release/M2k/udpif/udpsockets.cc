#include "udpsockets.h"


void            s_strf2c();

/* Machine-independent definitions */

/* To allow this module to be used both from C and C++, error handling is done
 * through a set of macros which allow the error to be noted either by
 * an exception being thrown, or by a call to perror(3c) and the return
 * of an illegal value. */
/* TODO(pab:29sep99): Add the alternative definitions so this can be used
 * outside of M2k. */

/* Declare the local variables which describe the error and its source */
#define SOCKERR_DECLARE(_fn) \
  int errNum = 0; \
  static char * errOp = #_fn; \
  string errDesc = ""

/* Record that we are in an error state.  This does not throw the
 * exception at this point; we may have additional cleanup to
 * perform. */
#define SOCKERR_RECORD(_d) \
  errNum = errno; \
  errDesc = (_d)

/* If an error has been recorded, throw an exception describing it;
 * otherwise return the suggested value. */
#define SOCKERR_RETURN(_rv) \
  if (0 < errDesc.length()) { \
    throw SocketLowLevelEx(errOp, errDesc, errNum); \
  } \
  return _rv


/* Function Declarations */

bool            FAILED(int status, const char *msg);

/******************************** CODE *****************************/


/* UDP_openPort

Description:
  Given a port number, creates and advertises a socket on that port.
  After the user has finished using the server, the 'server' pointer 
  should be given to destroyServer.

Arguments:
  server_h (OUT)	-- a handle.  A pointer to a pointer to a
			   server_type structure.
  port_num (IN+OUT)	-- UDP port number; [ >= 0 ];  if port_num is
                           zero, one will be assigned.

Returns:
  Zero if all went well; -1 or throw exception if there was an error.

NOTES:
  
*/

#include <signal.h>

#if defined(DECUNIX_)
#define socklen_t int
#endif

int_4 UDPSocketLib::UDP_openPort (server_ptr *server_h,
				  int_4      &port_num)
{
  SOCKERR_DECLARE(UDP_openPort);
  
  /* Because the client can choose to abort poorly,
   * we can get a sigpipe generated from which we want
   * to clean up from.  By ignoring it, the writes SHOULD
   * return a PIPEERR which we can do something with 
   */
  signal(SIGPIPE, SIG_IGN);

  int             status;
  server_ptr      server = NULL;

  server = (server_ptr) malloc(sizeof(server_type));
  if (!server) {
    GlobalLog.warning( "Could not allocate memory for server");
    SOCKERR_RECORD("Could not allocate memory for server");
    SOCKERR_RETURN(-1);
  }

  server->port_num = port_num;
  server->block = 0;
  server->socket = -1;

  const int on = 1;

  server->socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (server->socket < 0) {
    SOCKERR_RECORD("socket");
    goto CLEANUP_UDPIP;
  }
  ioc[server->socket].trans = 0;

  server->addrname.sin_family = AF_INET;
  server->addrname.sin_port = htons((u_short) port_num);
  server->addrname.sin_addr.s_addr = htonl(INADDR_ANY);

  /* From Stevens (p.288): "...we always use the SO_REUSEADDR socket
     option for a TCP server." */
  
  /* SunOS 5.5.1 requires the 4th argument to be a char *, but most
     other platforms require the 4th arg to be a void * and support
     char * for backward compatibility only. */
  
  status = setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  
  if (status < 0) {
    SOCKERR_RECORD("setsockopt(SO_REUSEADDR)");
    goto CLEANUP_UDPIP;
  }

  /* Bind the socket to the named port */

  status = bind(server->socket,
		((struct sockaddr *) & (server->addrname)),
		sizeof(server->addrname));

  if (status < 0) {
    SOCKERR_RECORD("bind");
    goto CLEANUP_UDPIP;
  }

  /* ill leave this for now...but don't need it... */
  if (server->port_num == 0) {

    /* Find out what our real name is */
    socklen_t namelen = sizeof(server->addrname);
    if (getsockname(server->socket,
		    (struct sockaddr*) & (server->addrname),
		    &namelen) < 0) {
      SOCKERR_RECORD("getsockname");
      goto CLEANUP_UDPIP;
    }
    server->port_num = ntohs((u_short)server->addrname.sin_port);
    //cout<<"UDP_openPort::Assigned Port="<<server->port_num<<endl;
    port_num = server->port_num;
  }
  
  *server_h = server;

  return 0;

 CLEANUP_UDPIP:
  if (server) {
    if (server->socket != -1) {
      UDP_closeSocket(server->socket);
    }
    free((char *) server);
  }
  SOCKERR_RETURN(-1);

}				/* UDP_openPort */



int_4 UDPSocketLib::descriptorForServer (server_type* server)
{
  return (server ? server->socket : -1);
}



int_4 UDPSocketLib::actualServerPort (server_type* server)
{
  return (server ? server->port_num : -1);
}



/* destroyServer

Description:
  Does the necessary cleaning-up to a server sockets.

Arguments:
  server (IN/OUT)		-- pointer to server

*/

void UDPSocketLib::destroyServer (server_ptr server)
{
  UDP_closeSocket(server->socket);

  (void) free((char *) server);
}



/* UDP_recv

Description:
  A short wrapper to the real socket reader.  Returns data read in buf
  parameter and # bytes read. 

Returns:
   0 to indicate no data available (NONBLOCKED sockets only).
  -1 or throw exception for an error.
  >0 Otherwise, returns the number of bytes read.

*/

int_4 UDPSocketLib::UDP_recv     (const int_4  sd, 
                                  unsigned char         *buf, 
                                  const int_4  bufsize,
                                  const int_u4 flags)
{
  SOCKERR_DECLARE(UDP_recv);
  int status;
  int ngot = 0;

#if SOLARIS_|SUN_
  status = recv (sd, (char *)buf, 
		 //(char *)bufsize, flags);
		 bufsize, flags);
#else
  status = recv (sd, buf, bufsize, flags);
#endif


  if (status < 0) {
    if (EWOULDBLOCK == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EAGAIN == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EINTR == socket_errno) {
      /* Were interrupted while reading, like maybe a condition variable
       * was kicked.  We might consider this a transitory problem, and
       * retry, or we might take this as a cue that we should pop back
       * up to the user and let them see if reality changed.  For now,
       * we'll treat it as transitory... */
    } else {
      /* Some other error.  Notify the caller of the error. */
      ngot = -1;
      perror("recv...");
      SOCKERR_RECORD("UDP_recv");
      SOCKERR_RETURN(status);
    }

  } else if (status > 0) {
    /* Read some non-zero number of bytes; record their existence
     * and keep going. */
    ngot = status;

  } else {
    /* Probably we reached EOF/no data situation.  Return with whatever 
     * we've got. Prob can't really reach here unless socket is NONBLOCK.*/
    return ngot;
  }

  return ngot;

}				/* UDP_recv */




/* UDP_recvFrom

Description:
  A short wrapper to the real socket reader.  Returns data read & 
  sender's IP address.

Returns:
   0 to indicate no data available (NONBLOCKED sockets only).
  -1 or throw exception for an error.
  >0 Otherwise, returns the number of bytes read.

*/

int_4 UDPSocketLib::UDP_recvFrom (const int_4         sd, 
                                  unsigned char       *buf, 
                                  const int_4         bufsize,
                                  const int_u4        flags, 
                                  struct sockaddr_in* clientAddr, 
                                  const int_4         clientAddrLength)
{
  SOCKERR_DECLARE(UDP_recvFrom);
  int status;
  int ngot = 0;
  socklen_t addrLength  = 0;

  addrLength = (int) clientAddrLength;
    
#if SOLARIS_|SUN_
  status = recvfrom (sd, (char *)buf, 
		     //(char *)bufsize, flags,
		     bufsize, flags,
		     (struct sockaddr *)clientAddr, &addrLength);
#else
  status = recvfrom (sd, buf, 
		     bufsize, flags,
		     (struct sockaddr *)clientAddr, &addrLength);
#endif


  if (status < 0) {
    if (EWOULDBLOCK == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EAGAIN == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EINTR == socket_errno) {
      /* Were interrupted while reading, like maybe a condition variable
       * was kicked.  We might consider this a transitory problem, and
       * retry, or we might take this as a cue that we should pop back
       * up to the user and let them see if reality changed.  For now,
       * we'll treat it as transitory... */
    } else {
      /* Some other error.  Notify the caller of the error. */
      ngot = -1;
      perror("recvfrom...");
      SOCKERR_RECORD("UDP_recvFrom");
      SOCKERR_RETURN(status);
    }

  } else if (status > 0) {
    /* Read some non-zero number of bytes; record their existence
     * and keep going. */
    ngot = status;

  } else {
    /* Probably we reached EOF/no data situation.  Return with whatever 
     * we've got. Prob can't really reach here unless socket is NONBLOCK.*/
    return ngot;
  }

  return ngot;

}				/* UDP_recvFrom */




/* UDP_recvFrom

Description:
  A short wrapper to the real socket reader.  Returns data bytes read 
  from UDP port & sender's Hostname and Port.

Returns:
   0 to indicate no data available (NONBLOCKED sockets only).
  -1 or throw exception for an error.
  >0 Otherwise, returns the number of bytes read.

*/

int_4 UDPSocketLib::UDP_recvFrom (const int_4   sd, 
                                  unsigned char *buf, 
                                  const int_4   bufsize,
                                  const int_u4  flags, 
                                  string        &host,
                                  int_4         &port)
{
  SOCKERR_DECLARE(UDP_recvFrom);
  int status;
  int ngot = 0;
  struct sockaddr_in   clientAddr;
  socklen_t  addrlen = sizeof(clientAddr);
  char client[62];

#if SOLARIS_|SUN_
  status = recvfrom (sd, (char *)buf, 
		     //(char *)bufsize, flags,
		     bufsize, flags,
		     (struct sockaddr *)&clientAddr, &addrlen);
#else
  status = recvfrom (sd, buf, 
		     bufsize, flags,
		     (struct sockaddr *)&clientAddr, &addrlen);
#endif

  if (status < 0) {
    if (EWOULDBLOCK == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EAGAIN == socket_errno) {
      /* No data available right now, on a non-blocking socket. */
    } else if (EINTR == socket_errno) {
      /* Were interrupted while reading, like maybe a condition variable
       * was kicked.  We might consider this a transitory problem, and
       * retry, or we might take this as a cue that we should pop back
       * up to the user and let them see if reality changed.  For now,
       * we'll treat it as transitory... */
    } else {
      /* Some other error.  Notify the caller of the error. */
      ngot = -1;
      perror("recvfrom...");
      SOCKERR_RECORD("UDP_recvFrom");
      SOCKERR_RETURN(status);
    }

  } else if (status > 0) {
    /* Read some non-zero number of bytes; record their existence
     * and keep going. */
    ngot = status;

    status = UDP_getHostByAddr(client, sizeof(client), &clientAddr);
    string tmpHost (client);
    host = client;
    port = ntohs(clientAddr.sin_port);

  } else {
    /* Probably we reached EOF/no data situation.  Return with whatever 
     * we've got. Prob can't really reach here unless socket is NONBLOCK.*/
    return ngot;
  }

  return ngot;

}				/* UDP_recvFrom */




/* UDP_sendTo

Description:
  A short wrapper to the real UDP socket writer. Retimplements some more
  convenient non-blocking modes and insulates the user from VMS imposed
  transfer length limitations.

Returns:
   0 to indicate no data transmitted (NONBLOCKED sockets only). OR
     strangely, app code did not specify any buffer length (bufsize=0).
  -1 or throw exception for an error.
  >0 Otherwise, returns the number of bytes sent.
*/

int_4 UDPSocketLib::UDP_sendTo (const int_4               sd, 
                                const unsigned char                *buf, 
                                const int_4               bufsize,
                                const int_u4              flags, 
                                const struct sockaddr_in* clientAddr, 
                                const int_4               addrlen)
{
  SOCKERR_DECLARE(UDP_sendTo);
  int status;
  int ngot = 0;

#if SOLARIS_|SUN_
  status = sendto (sd, (char *)buf, 
		   //(char *)bufsize, flags);
		   bufsize, flags,
		   (struct sockaddr *)clientAddr, addrlen);
#else
  status = sendto (sd, buf, 
		   bufsize, flags,
		   (struct sockaddr *)clientAddr, addrlen);
#endif

  if (status < 0) {
    if (EWOULDBLOCK == socket_errno) {
      /* No space avail to write data to UDP buffer on a non-blocking socket. */
      ngot = 0;
    } else if (EAGAIN == socket_errno) {
      /* No space avail to write data to UDP buffer on a non-blocking socket. */
      ngot = 0;
    } else if (EINTR == socket_errno) {
      /* Interrupt signal recvd while writing, before data was xmitted.
       * We might consider this a transitory problem, and retry,
       * or we might take this as a cue that we should pop back
       * up to the user and let them see if reality changed.  For now,
       * we'll treat it as transitory... */
      ngot = 0;
    } else {
      /* Some other error.  Notify the caller of the error. */
      perror("UDP_sendTo <clientAddr>...");
      SOCKERR_RECORD("Failed <sendto> call;");
      SOCKERR_RETURN(status);
    }

  } else if (status > 0) {
    /* Read some non-zero number of bytes; record their existence
     * and keep going (lets blow this joint...). */
    ngot = status;

  } else {
    /* Probably we reached EOF.  Return with whatever we've got. 
     * A very unlikely place to end up as "sendto" does not return a 0
     * unless of course you sent it a zero (0).  */
    return ngot;
  }

  return ngot;

}				/* UDP_sendTo */




/* UDP_sendTo

Description:
  A short wrapper to the real UDP socket writer. Retimplements some more
  convenient non-blocking modes and insulates the user from VMS imposed
  transfer length limitations.

Returns:
   0 to indicate no data transmitted (NONBLOCKED sockets only). OR
     strangely, app code did not specify any buffer length (bufsize=0).
  -1 or throw exception for an error. Could not find <host> specified
     or could not write to UDP send buffer.
  >0 Otherwise, returns the number of bytes sent.
*/

int_4 UDPSocketLib::UDP_sendTo (const int_4   sd, 
                                const unsigned char    *buf, 
                                const int_4   bufsize,
                                const int_u4  flags, 
                                const string  destHost,
                                const int_4   destPort)
{
  SOCKERR_DECLARE(UDP_sendTo);
  char                hname[MAX_HOSTNAME_LENGTH];
  struct sockaddr_in  destAddr;
  int                 addrlen = sizeof(destAddr);
  int                 status;
  int                 ngot    = 0;

  status = UDP_findHostByName(destHost.c_str(), 
			      hname, 
			      sizeof(hname), 
			      &destAddr);
  if (status < 0) {
    perror("UDP_sendTo:UDP_findHostByName()...");
    SOCKERR_RECORD("Failed <UDP_findHostByName> call;");
    SOCKERR_RETURN(status);
  }

  destAddr.sin_port = htons((u_short) destPort);

#if SOLARIS_|SUN_
  status = sendto (sd, (char *)buf, 
		   //(char *)bufsize, flags);
		   bufsize, flags,
		   (struct sockaddr *)&destAddr, addrlen);
#else
  status = sendto (sd, buf, 
		   bufsize, flags,
		   (struct sockaddr *)&destAddr, addrlen);
#endif

  if (status < 0) {
    if (EWOULDBLOCK == socket_errno) {
      /* No space avail to write data to UDP buffer on a non-blocking socket. */
      ngot = 0;
    } else if (EAGAIN == socket_errno) {
      /* No space avail to write data to UDP buffer on a non-blocking socket. */
      ngot = 0;
    } else if (EINTR == socket_errno) {
      /* Interrupt signal recvd while writing, before data was xmitted.
       * We might consider this a transitory problem, and retry,
       * or we might take this as a cue that we should pop back
       * up to the user and let them see if reality changed.  For now,
       * we'll treat it as transitory... */
      ngot = 0;
    } else {
      /* Some other error.  Notify the caller of the error. */
      perror("UDP_sendTo <destHost:destPort>...");
      SOCKERR_RECORD("Failed <sendto> call;");
      SOCKERR_RETURN(status);
    }

  } else if (status > 0) {
    /* Read some non-zero number of bytes; record their existence
     * and keep going (lets blow this joint...). */
    ngot = status;

  } else {
    /* Probably we reached EOF.  Return with whatever we've got. 
     * A very unlikely place to end up as "sendto" does not return a 0
     * unless of course you sent it a zero (0).  */
    return ngot;
  }

  return ngot;

}				/* UDP_sendTo */




/* UDP_closeSocket

Description:
  Closes the socket connection associated with <sd>.

*/

void UDPSocketLib::UDP_closeSocket (int_4 sd)
{
  SOCKERR_DECLARE(UDP_closeSocket);

  /* TODO(pab:12oct99): Although historically we've ignored any errors that
   * arise on closing a socket, this is a Bad Idea In General.  But since
   * most close(2) failures occur only when writing to disk files, and most
   * people probably won't know what to do in that situation anyway, the
   * only thing we'll do is retry as long as we're getting interrupted by a
   * signal. */

  int rc;
  do {
    rc = socket_close(sd);
  } while ((-1 == rc) && (EINTR == socket_errno));

  return;

}				/* UDP_closeSocket */




/* UDP_unblockSocket

Description:
  Given a socket descriptor, configures it so that it will either 
  NOT block when it reads or writes.

Arguments:
  sd	    (IN)	-- the socket descriptor

Returns:
  -1		if error was encountered
   0		if all went well

*/

int_4 UDPSocketLib::UDP_unblockSocket (int_4 sd)
{
  SOCKERR_DECLARE(UDP_unblockSocket);
  int             status;
  int             blockFlag (NONBLOCK);

  // may need to timeout protect all loops like this...
  //do {
  status = socket_ioctl(sd, FIONBIO, (char *) &blockFlag);
  //} while ((-1 == status) && (EINTR == socket_errno));

  if (status < 0) {
    SOCKERR_RECORD("ioctl(FIONBIO)");
  }
  SOCKERR_RETURN(status);
}				/* UDP_unblockSocket */




/* UDP_blockSocket

Description:
  Given a socket descriptor, configures it so that it will either 
  NOT block when it reads or writes.

Arguments:
  sd	    (IN)	-- the socket descriptor

Returns:
  -1		if error was encountered
   0		if all went well

*/

int_4 UDPSocketLib::UDP_blockSocket (int_4 sd)
{
  SOCKERR_DECLARE(UDP_blockSocket);
  int             status;
  int             blockFlag (BLOCK);

  // may need to timeout protect all loops like this...
  do {
    status = socket_ioctl(sd, FIONBIO, (char *) &blockFlag);
  } while ((-1 == status) && (EINTR == socket_errno));

  if (status < 0) {
    SOCKERR_RECORD("ioctl(FIONBIO)");
  }
  SOCKERR_RETURN(status);
}				/* UDP_blockSocket */




/* UDP_setBlockStatus

Description:
  Given a socket descriptor, configures it so that it will either block
  when it reads or writes.

Arguments:
  sd	    (IN)	-- the socket descriptor
  blockFlag (IN)	-- Block flag
                             BLOCK     (=0) for blocked reads/writes
                             NOBLOCK   (=1) for unblocked reads/writes
Returns:
  -1		if error was encountered
   0		if all went well

*/

int_4 UDPSocketLib::UDP_setBlockStatus (int_4 sd, int_4 blockFlag)
{
  SOCKERR_DECLARE(UDP_setBlockStatus);
  int             status;

  // may need to timeout protect all loops like this...
  do {
    status = socket_ioctl(sd, FIONBIO, (char *) &blockFlag);
  } while ((-1 == status) && (EINTR == socket_errno));
  if (status < 0) {
    SOCKERR_RECORD("ioctl(FIONBIO)");
  }
  SOCKERR_RETURN(status);
}				/* UDP_setBlockStatus */




/* UDP_setBufferSize

Description:
  Sets the size of the underlying buffer, or "transfer window", on the given
  socket.

Arguments:
  sd (IN)		-- socket descriptor
  option (IN)		-- string specifying which buffer to set.
				"r"  specifies the reading buffer,
				"w"  specifies the writing buffer,
				"rw" specifies both.
  bufsize (IN)		-- the desired size of the buffer

Returns:
  0 on success; -1 on an error.

*/

int_4 UDPSocketLib::UDP_setBufferSize (int_4 sd, const char *option, int_4 bufsize)
{
  SOCKERR_DECLARE(UDP_setBufferSize);
  int             status, i;


  status = 0;
  for (i = 0; option[i] != '\0'; i++) {
    switch (option[i]) {
    case 'r':
      status = setsockopt(sd, SOL_SOCKET, SO_RCVBUF,
#if SOLARIS_|SUN_
			  (char *)bufsize, sizeof(int_4));
#else
      &bufsize, sizeof(int_4));
#endif
    if (status < 0) {
      SOCKERR_RECORD("setsockopt(SO_RCVBUF)");
    }
    break;

  case 'w':
    status = setsockopt(sd, SOL_SOCKET, SO_SNDBUF,
#if SOLARIS_|SUN_
			(char *)bufsize, sizeof(int_4));
#else
    &bufsize, sizeof(int_4));
#endif
  if (status < 0) {
    SOCKERR_RECORD("setsockopt(SO_SNDBUF)");
  }
  break;
 default:
   SOCKERR_RECORD("illegal character in option string '" + string(option) + "'");
   break;
}
}
SOCKERR_RETURN(status);
}				/* UDP_setBufferSize */



/* UDP_getBufferSize

Description:
  Sets the size of the underlying buffer, or "transfer window", on the given
  socket.

Arguments:
  sd (IN)		-- socket descriptor
  option (IN)		-- string specifying which buffer to set.
				"r"  specifies the reading buffer,
				"w"  specifies the writing buffer,

Returns:
  0 on success; -1 on an error.

*/

int_4 UDPSocketLib::UDP_getBufferSize (int_4 sd, const char *option)
{
SOCKERR_DECLARE(UDP_getBufferSize);
int             status;
int             size = 0;
socklen_t       optlen;


status = 0;

switch (option[0]) {
case 'r':
optlen = sizeof (size);
status = getsockopt(sd, SOL_SOCKET, SO_RCVBUF,
  (char *) &size, &optlen);
if (status < 0) {
SOCKERR_RECORD("getsockopt(SO_RCVBUF)");
}
break;

case 'w':
optlen = sizeof (size);
status = getsockopt(sd, SOL_SOCKET, SO_SNDBUF,
      (char *) &size, &optlen);
if (status < 0) {
SOCKERR_RECORD("getsockopt(SO_SNDBUF)");
}
break;
default:
SOCKERR_RECORD("illegal option'" + string(option) + "'");
break;
}

SOCKERR_RETURN(status);

}				/* UDP_getBufferSize */



/* UDP_findHostByName

Description:
  A wrapper to get the commonly desired values out of gethostbyname,
  which generally is not thread safe, in a thread-safe manner where
  supported by a given host.

Arguments:
  hostname (IN) -- ASCIIZ name of host to look up
  h_name_p (OUT) -- canonical name of host
  h_name_l (IN) -- maximum number of chars to store for host name, incl EOS
  sap (OUT) -- pointer to a sockaddr_in buffer to hold address/family info

Returns:
  Zero if all went well; -1 if there was an error.

NOTES:
  + The user is responsible for making sure the output buffers are large
    enough
  + If the reference pointers are NULL, the corresponding outputs are
    not set.
*/

int_4 UDPSocketLib::UDP_findHostByName (const char *hostname,
      char *h_name_p,
      int_4 h_name_l,
      struct sockaddr_in* sap)
{
  struct hostent * hp;
  
  /* For those of you using purify, gethostbyname seems to use up a 
   * socket on solaris (probably to talk to the dns server).  This
   * is not a problem. */
  hp = gethostbyname(hostname);

  if (0 == hp) {
    return -1;
  }

  if (0 != h_name_p) {
    strncpy(h_name_p, hp->h_name, h_name_l);
    h_name_p[h_name_l-1] = 0;	/* Force EOS termination */
  }
  if (0 != sap) {
    sap->sin_family = hp->h_addrtype;
    memcpy(&sap->sin_addr, hp->h_addr_list[0], hp->h_length);
  }

  return 0;
}				/* UDP_findHostByName */





int_4 UDPSocketLib::UDP_getHostByAddr (char *h_name_p,
                                       int_4 h_name_l,
                                       struct sockaddr_in* sap)
{
  struct hostent * hp;
  
  hp = gethostbyaddr((char *)&sap->sin_addr, 4, AF_INET);

  //printf("\nCLIENT = [%s (%s):%d]\n",
  //       hp->h_name, 
  //       inet_ntoa(sap->sin_addr),
  //       ntohs(sap->sin_port));

  if (0 == hp) {
    return -1;
  }

  if (0 != h_name_p) {
    strncpy(h_name_p, hp->h_name, h_name_l);
    h_name_p[h_name_l-1] = 0;	/* Force EOS termination */
  }

  return 0;
}				/* UDP_getHostByAddr */




/* The wrappers that throw away the useful error information to be
 * compatible with the previous interface. */

int_4 m_udp_create_server (server_type **server_h,
			   int_4 port_num)
{
  try {
    return UDPSocketLib::UDP_openPort(server_h, port_num);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

int_4 m_udp_descriptor_for_server (server_type* server)
{
  try {
    return UDPSocketLib::descriptorForServer(server);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

int_4 m_udp_actual_server_port (server_type* server)
{
  try {
    return UDPSocketLib::actualServerPort(server);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

void m_udp_destroy_server (server_type* server)
{
  try {
    UDPSocketLib::destroyServer(server);
  } catch (const SocketLowLevelEx& e) {
  }
}

int_4 m_udp_read_socket (int_4 sd, unsigned char *buf, int_4 bytes)
{
  int_u4  flags = 0;
  struct  sockaddr_in clientAddr;
  int_4   addrlen = sizeof(clientAddr);
  try {
    return UDPSocketLib::UDP_recvFrom (sd, 
                                       buf, 
                                       bytes, 
                                       flags, 
                                       &clientAddr, 
                                       addrlen);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

void m_udp_close_socket (int_4 sd)
{
  try {
    UDPSocketLib::UDP_closeSocket(sd);
  } catch (const SocketLowLevelEx& e) {
  }
}

int_4 m_udp_set_block_status (int_4 sd, int_4 toggle)
{
  try {
    return UDPSocketLib::UDP_setBlockStatus (sd, toggle);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

int_4 m_udp_set_buffer_size (int_4 sd, const char *option, int_u4 bufsize)
{
  try {
    return UDPSocketLib::UDP_setBufferSize (sd, option, bufsize);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}

int_4 m_udp_findhostbyname (const char *hostname,
			    char *h_name_p, int_4 h_name_l,
			    struct sockaddr_in* sap)
{
  try {
    return UDPSocketLib::UDP_findHostByName(hostname, h_name_p, h_name_l, sap);
  } catch (const SocketLowLevelEx& e) {
    return -1;
  }
  /*NOTREACHED*/
}


