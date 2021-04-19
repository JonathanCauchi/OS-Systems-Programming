#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
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

int main( int argc, char *argv[] )
{
    // Declare variables
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int  n;
	
	// Create buffer to store incoming particles
	char *buffer = (char *) malloc(sizeof(particle_protocol));

    // Create server socket (AF_INET, SOCK_STREAM)
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR opening socket");
		free(buffer);
        exit(1);
    }

    // Initialize socket structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;  // Server port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections from any address
    serv_addr.sin_port = htons(portno);
 
    // Bind the host address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
		 free(buffer);
		 close(sockfd);
         exit(1);
    }

    // Start listening for the clients
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Accept actual connection from the client
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) 
    {
        perror("ERROR on accept");
		free(buffer);
		close(sockfd);
        exit(1);
    }

    // If connection is established then start communicating 
    bzero(buffer, sizeof(particle_protocol));
    if ((n = read(newsockfd, buffer, sizeof(particle_protocol) )) < 0)
    {
        perror("ERROR reading from socket");
		free(buffer);
		close(sockfd);
		close(newsockfd);
        exit(1);
    }
	
	// We have particle data, extract information
	particle_protocol *pp = (particle_protocol *) buffer;
	
    // Check how many particles we have received
	int nof_particles = ntohl(pp->nof_particles);
	printf("We have received %d particles\n", nof_particles);
	
	// Print out particle info
	for(unsigned i = 0; i < nof_particles; i++)
		printf("Particle %d:\tx=%.0f\ty=%.0f\tz=%.0f\n", i, (float) ntohl(pp->particles[i].x), 
													        (float) ntohl(pp->particles[i].y), 
													        (float) ntohl(pp->particles[i].z));
	

    // All done, close sockets
    close(newsockfd);
    close(sockfd);
    return 0; 
}
