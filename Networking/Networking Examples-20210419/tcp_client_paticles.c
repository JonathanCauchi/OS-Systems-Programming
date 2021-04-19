#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_PARTICLES 128

typedef struct p
{
	float x, y, z;
} particle;

typedef struct pp
{
	int nof_particles;
	particle particles[MAX_PARTICLES];
} particle_protocol;

int main(int argc, char *argv[])
{
    // Declare variables
    int sockfd, portno, n, nof_particles;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Make sure that server name and port are available in command line arguments
    if (argc < 4) {
        fprintf(stderr,"usage %s hostname port nof_particles\n", argv[0]);
        exit(0);
    }

    // Get port number 
    portno = atoi(argv[2]);
	
	// Get number of particles
	if ((nof_particles = atoi(argv[3])) > MAX_PARTICLES)
	{    
        fprintf(stderr, "Maximum number of particles allowed is %d (got %d)\n", MAX_PARTICLES, nof_particles);
		exit(0);
	}

    // Create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Get server name 
    if ((server = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
		close(sockfd);
        exit(0);
    }

    // Populate serv_addr structure 
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  // Set to AF_INET
    bcopy((char *) server -> h_addr, // Set server address
          (char *) &serv_addr.sin_addr.s_addr,
                   server -> h_length);
    serv_addr.sin_port = htons(portno); // Set port (convert to network byte ordering)

    // Connect to the server 
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
         perror("ERROR connecting");
		 close(sockfd);
         exit(1);
    }	

    // Create particle array
	particle_protocol protocol;
	protocol.nof_particles = htonl(nof_particles);
	
	for(unsigned i = 0; i < nof_particles; i++)
	{
		protocol.particles[i].x = htonl(i);
		protocol.particles[i].y = htonl(i * i);
		protocol.particles[i].z = htonl(i * i * i);
	}

    // Send message to the server
    if ((n = write(sockfd, (char *) &protocol, sizeof(protocol)) < 0))
    {
         perror("ERROR writing to socket");
		 close(sockfd);
         exit(1);
    }


    // All done, close socket
    close(sockfd);
    return 0;
}
