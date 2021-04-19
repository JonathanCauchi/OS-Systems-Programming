#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>

#define PORT    5001
#define MAXMSG  512

int read_from_client(int filedes) {
    char buffer[MAXMSG];
    int nbytes;

    nbytes = read(filedes, buffer, MAXMSG);
    if (nbytes < 0) {
        // Read error
        perror("read");
        close(filedes);
        exit(EXIT_FAILURE);
    } else if (nbytes == 0)
        // End-of-file.
        return -1;
    else {
        // Data read.
        fprintf(stderr, "Server: got message: `%s'\n", buffer);

        if ((nbytes = write(filedes, "I got your message", 18)) < 0) {
            close(filedes);
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }
        return 0;
    }
}

int main(void) {
    // Declare variables
    int sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    struct sockaddr_in clientname, serv_addr;
    int size;

    // Create server socket (AF_INET, SOCK_STREAM)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // Allow address reuse
    int opts = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

    // Gte current options
    if ((opts = fcntl(sock, F_GETFL)) < 0) {
        perror("Error getting socket options\n");
        close(sock);
        exit(1);
    }

    // Set socket to non-blocking
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0) {
        {
            perror("Error setting socket to non-blocking");
            close(sock);
            exit(1);
        }
    }

    // Initialize socket structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
    serv_addr.sin_port = htons(PORT);

    // Bind the host address
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        close(sock);
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        perror("listen");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

    while (1) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select");
            close(sock);
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == sock) {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *) &clientname, &size);
                    if (new < 0) {
                        close(sock);
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr,
                            "Server: connect from host %s, port %d.\n",
                            inet_ntoa(clientname.sin_addr),
                            ntohs(clientname.sin_port));
                    FD_SET (new, &active_fd_set);
                } else {
                    /* Data arriving on an already-connected socket. */
                    if (read_from_client(i) < 0) {
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }
}

