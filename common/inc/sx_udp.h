#if !defined(_UDP_SOCKET_H_)
#define _UDP_SOCKET_H_

#define SBOX_UDP_ID void *

extern void * sx_udp_create(
    unsigned short  local_port
    );

extern void sx_udp_send(
    SBOX_UDP_ID     id,
    unsigned char  *pkt,
    unsigned int    pkt_len
    );

extern void sx_udp_recv(
    SBOX_UDP_ID     id,
    char           *pkt,
    unsigned int   *pkt_len
    );

#endif // #if !defined(_UDP_SOCKET_H_)
