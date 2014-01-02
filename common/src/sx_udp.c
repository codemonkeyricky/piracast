// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <errno.h> 

#include "sx_types.h"
#include "sx_udp.h"

typedef struct
{
    unsigned int    peer_ip;
    unsigned short  peer_port;
    unsigned int    local_ip;
    unsigned short  local_port;

    int             sock;

} sUDP_CBLK;


void * sx_udp_create(
    UINT16 port
    )
{
    struct sockaddr_in  client_addr;
    int                 rv;


    sUDP_CBLK  *udp_cblk = malloc(sizeof(sUDP_CBLK));

    memset(udp_cblk, 0, sizeof(sUDP_CBLK));

    // Create socket.
    udp_cblk->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(udp_cblk->sock != -1);
    
    #if 0
    char *opt;
    opt = "wlan0";
    rv = setsockopt(udp_cblk->sock, SOL_SOCKET, SO_BINDTODEVICE, opt, 5);
    assert(rv == 0);
    #endif 

    // Listen to anyone that talks to us on the right port.
    client_addr.sin_family      = AF_INET;
    client_addr.sin_port        = htons(port);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(client_addr.sin_zero), 8);

    // Bind to address.
    rv = bind(udp_cblk->sock,
              (struct sockaddr *) &client_addr,
              sizeof(client_addr)); 
    if(rv < 0)
    {
        // printf("(udp): udp_socket_create(): rv = %d, errno %s\n", rc, strerror(errno));
        assert(0); 
    }

    return udp_cblk;
}


void sx_udp_recv(
    SBOX_UDP_ID     id,
    char           *pkt,
    unsigned int   *pkt_len
    )
{
    struct sockaddr_in  server_addr;
    unsigned int        bytes_read;
    unsigned int        addr_len;
    sUDP_CBLK          *udp_cblk = id;


#if 0
    printf("(udp): udp_socket_recv(): ID = 0x%x\n", id);
#endif

    addr_len = sizeof(struct sockaddr);
//    printf("(udp): pkt = 0x%x, pkt_len = %d\n", pkt, *pkt_len);
//    printf("(udp): before\n");
    bytes_read = recvfrom(udp_cblk->sock,
                          pkt,
                          *pkt_len,
                          0,
                          (struct sockaddr *) &server_addr,
                          &addr_len);
//    printf("(udp): after\n");
    if(bytes_read <= 0)
    {
#if 0
        printf("(udp): udp_socket_recv(): error = %d\n", bytes_read);
#endif
        assert(0);
    }

#if 0
    printf("(udp): udp_socket_recv()\n");
#endif

    *pkt_len = bytes_read;
}


void sx_udp_destroy(
    SBOX_UDP_ID     id
    )
{
    // TODO:
}
