#ifndef ITXUDPIF_H_

//
//
// Author: C Pendergraph
//         largely derived from XMSocketMsg by Steve Capie
//         UDP version of that code

// ///////////////////////////////////////////// Include Files

// ///////////////////////////////////////////// ITXUDPIF

// ///////////////////////////////////////////// Constants

  const real_8   KeepAliveInterval      (10.0);
  const real_8   ReconnectInterval      ( 5.0);
  const real_8   ServerTimeoutInterval  (30.0);

  const int_u4  UDPDBG_NETIN  (0x0001);       //0000 0000 0000 0001//
  const int_u4  UDPDBG_NETOUT (0x0002);       //0000 0000 0000 0010//
  const int_u4  UDPDBG_DETAIL (0x0004);       //0000 0000 0000 0100//
  const int_u4  UDPDBG_TIMERS (0x0008);       //0000 0000 0000 1000//

  const int_2 UDP_MSG_CLIENT_CONNECT_REQ = 1111;
  const int_2 UDP_MSG_CLIENT_DISCONNECT  = 2222;
  const int_2 UDP_MSG_DATA               = 3333;
  const int_2 UDP_MSG_SERVER_DISCONNECT  = 4444;
  const int_2 UDP_MSG_CONNECT_REQ_ACK    = 5555;
  const int_2 UDP_MSG_KEEP_ALIVE         = 6666;

// ///////////////////////////////////////////// Type Definitions

  // size= 24
  struct XintHeader {
    int_2      length;
    int_2      type;
    int_u4     sia;
    int_u2     port;
    int_2      id;
    int_2      fill[2];
    real_8     time;
  };

  const int_4 MAX_UDP_DATAGRAM_SIZE = 8192;

  // size= 8192
  struct XinterimUDPMsg {
    XintHeader        XINT_hdr;
  };

  // size= 8192; note its a UNION
  union UDPdatagram {
    XinterimUDPMsg   XINTmsg;
    int_u1           buffer [MAX_UDP_DATAGRAM_SIZE];
  };

  const int_4 MAX_UDP_DATA = sizeof(UDPdatagram);


#define ITXUDPIF_H_
#endif // ITXUDPIF_H_
