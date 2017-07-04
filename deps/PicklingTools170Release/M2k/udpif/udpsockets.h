#ifndef UDPSOCKETS_H_

//
//

/*

  @(#)

				Tucson, Arizona

Description:
  A set of routines which provide a interface between UDP sockets 
  and C/C++ code.

Author:   C Pendergraph   13 March 2000;  near the ides of March,
                                          in this cold, wintry month in
                                          N Yorkshire.

  Borrowed and based heavily upon "sockets.cc" and "sockets.h" by 
  Derek Jones (RRC) & Scott Schoen (RRC).  Removed TCP "connection"
  and stream stuff. Also removed any support for DECNET/UCX.

  If code seems half-way like C and half-way like C++, its because it 
  is.  Scott & Derick had C compatible design criteria that ive aborted
  it favor of function over-loading & more restrictive typing.  Could 
  quite easily be reverted back to C if necessary, though unlikely.

Compilation:
  This module has been compiled/tested on the Sun under Solaris and the
  DEC Alpha under OSF/1, with Midas2K 2.1.27 and above.

  These macros can have alternative definitions when this file is to
  be compiled in a C-only environment.
  
  * To provide an interface that looks like the original m_* routines
  for people who are calling these directly from C++ code, every
  routine has a wrapped version which traps the exception and instead
  returns a -1.  This requirement forces us to change the name of the
  exception-throwing routine.

*/





#define BLOCK    0
#define NONBLOCK 1

/* Standard includes */
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>


#if !defined(DECUNIX_) && !defined(LINUX_)
#include <sys/filio.h>			/* for System V? */
#endif

#if DECUNIX_
#include <stdlib.h>


#else
#include <malloc.h>


#endif

/* Midas includes */
#include "m2compiler.h"
#include "m2globallog.h"
#include "m2cctypes.h"
#include "m2sockete.h"


#define ATTR
//#define TCPIP 1


#define  socket_read   read
#define  socket_write  write
#define  socket_close  close
#define  socket_ioctl  ioctl
#define  socket_errno  errno
#define  socket_perror perror
#define  socket_fcntl  fcntl


/* Type Definitions */
typedef struct {
  short           chan, state;
  char            block, trans;
} chan_struct;


typedef struct server_struct {
  struct sockaddr_in addrname;
  int             socket;
  int             port_num;
  short           mbxchan, block;
  int		  useAcceptTimeout;
  struct timeval  acceptTimeout;
} server_type;

typedef server_type *server_ptr;

/* Global Declarations */
#define L_transfer 16384	        /* max socket xfer length per read or
				         * write */
#define L_port 512		        /* max socket ports open per image */
ATTR static chan_struct ioc[L_port];	/* array to hold socket parameters */

#define MAX_HOSTNAME_LENGTH 62

//==========================================================

// Forward reference
struct server_struct;
typedef server_struct server_type;

// Forward reference
struct sockaddr_in;



class UDPSocketLib
{
  public:
    // throws SocketLowLevelEx
    static int_4 UDP_openPort (server_type **server_h,
			       int_4       &port_num);

    // does not throw on error
    static int_4 descriptorForServer (server_type* server);

    // does not throw on error
    static int_4 actualServerPort (server_type* server);

    // does not throw on error
    static void destroyServer (server_type* server);

    // throws SocketLowLevelEx
    static int_4 UDP_recv     (const int_4         sd, 
                               unsigned char       *buf, 
                               const int_4         bytes,
                               const int_u4        flags);

    // throws SocketLowLevelEx
    static int_4 UDP_recvFrom (const int_4         sd, 
                               unsigned char       *buf, 
                               const int_4         bytes,
                               const int_u4        flags,
			       struct sockaddr_in* sap,
                               const int_4         addrlen);

    // throws SocketLowLevelEx
    static int_4 UDP_recvFrom (const int_4         sd, 
                               unsigned char       *buf, 
                               const int_4         bytes,
                               const int_u4        flags,
			       string              &host,
                               int_4               &port);

    // throws SocketLowLevelEx
    static int_4 UDP_sendTo   (const int_4               sd, 
                               const unsigned char       *buf, 
                               const int_4               bytes,
                               const int_u4              flags,
			       const struct sockaddr_in* sap,
                               const int_4               addrlen);

    // throws SocketLowLevelEx
    static int_4 UDP_sendTo   (const int_4         sd, 
                               const unsigned char *buf, 
                               const int_4         bytes,
                               const int_u4        flags,
			       const string        host,
                               const int_4         port);

    // does not throw on error
    static void  UDP_closeSocket (int_4 sd);

    // throws SocketLowLevelEx
    static int_4 UDP_unblockSocket (int_4 sd);
    static int_4 UDP_blockSocket   (int_4 sd);
    static int_4 UDP_setBlockStatus (int_4 sd, int_4 blockFlag);

    // throws SocketLowLevelEx
    static int_4 UDP_setBufferSize (int_4 sd, const char *option, int_4 bufsize);

    // throws SocketLowLevelEx
    static int_4 UDP_getBufferSize (int_4 sd, const char *option);

    // throws SocketLowLevelEx
    static int_4 UDP_findHostByName (const char *hostname,
				     char *h_name_p, int_4 h_name_l,
				     struct sockaddr_in* sap);
    // throws SocketLowLevelEx
    static int_4 UDP_getHostByAddr (char *h_name_p, int_4 h_name_l,
				    struct sockaddr_in* sap);
};

int_4 m_create_server         (server_type **server_h, int_4 port_num);
int_4 m_descriptor_for_server (server_type* server);
int_4 m_actual_server_port    (server_type* server);
void  m_destroy_server        (server_type* server);
int_4 m_read_socket           (int_4 sd, char *buf, int_4 bytes);
void  m_close_socket          (int_4 sd);
int_4 m_unblock_socket        (int_4 sd, int_4 toggle);
int_4 m_buffer_socket         (int_4 sd, const char *option, int_u4 bufsize);
int_4 m_findhostbyname        (const char *hostname, char *h_name_p, int_4 h_name_l,
			       struct sockaddr_in* sap);

#define UDPSOCKETS_H_
#endif
