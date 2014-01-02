#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>

#include "sx_types.h"
#include "sx_mgmt_ctl.h"

void sx_mgmt_ctl_init(
    void
    )
{
}


int connection_handler(
    int connection_fd
    )
{
    int nbytes;
    char buffer[256];


    printf("mgmt_ctl_wait(): Reading incoming message...\n"); 

    nbytes = read(connection_fd, buffer, 256);
    buffer[nbytes] = 0;

    printf("MESSAGE FROM CLIENT: %s\n", buffer);
//    nbytes = snprintf(buffer, 256, "hello from the server");
//    write(connection_fd, buffer, nbytes);

    close(connection_fd);

    return 0;
}


void sx_mgmt_ctl_wait(
    void
    )
{
    struct sockaddr_un address;
    int socket_fd, connection_fd;
    socklen_t address_length;
    pid_t child;

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        printf("socket() failed\n");
        return;
    }

    unlink("./demo_socket");

    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, 108, "./demo_socket");

    if(bind(socket_fd,
            (struct sockaddr *) &address,
            sizeof(struct sockaddr_un)) != 0)
    {
        printf("bind() failed\n");
        return;
    }

    printf("mgmt_ctl_wait(): Waiting for incoming connection...\n"); 

    if(listen(socket_fd, 5) != 0)
    {
        printf("listen() failed\n");
        return;
    }

    printf("mgmt_ctl_wait(): Accept new connection.\n"); 

    connection_fd = accept(socket_fd,
                           (struct sockaddr *) &address,
                           &address_length);

    /* now inside newly created connection handling process */
    connection_handler(connection_fd);

    /* still inside server process */
    close(connection_fd);

    close(socket_fd);
    unlink("./demo_socket");
}
