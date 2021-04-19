#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void dostuff(int); /* function prototype */

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	 // Declare variables
     int sockfd, newsockfd, portno, clilen, pid;
     struct sockaddr_in serv_addr, cli_addr;

	 // Get port from command line
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
	 
	 // Create server socket (AF_INET, SOCK_STREAM)
     if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) , 0)
        error("ERROR opening socket");
	
	 // Initialize socket structure
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
	 
	 // Bind the host address
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		 close(sockfd);
         error("ERROR on binding");
	 }
	 
	 // Start listening for the clients
     listen(sockfd,5);
	 
	 // Infinite loop
     clilen = sizeof(cli_addr);
     while (1) {
		 
		 // Accept connection form a client
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		 
         if (newsockfd < 0)  {
			 close(sockfd);
             error("ERROR on accept");
		 }
		 
		 // Accept successful, fork a new process
         pid = fork();
		
         if (pid < 0) {
			 close(sockfd);
			 close(newsockfd);
             error("ERROR on fork");
		 }
		 
		 // If this is the child, serve client
         if (pid == 0)  {
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
		 
		 // Otherwise this is the parent, close the new socket
		 // and wait for another client
         else 
			 close(newsockfd);
		 
		 // NOTE: you should call wait for created processes to 
		 // clear out child data in OS
		 
     } // End while
	 
     return 0; // we never get here
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n;
   char buffer[256];
      
   bzero(buffer,256);
   
   if ((n = read(sock,buffer,255)) < 0) {
	   close(sock);
	   error("ERROR reading from socket");
   }
   
   sleep(5);
   
   printf("Here is the message: %s\n",buffer);
   
   if ((n = write(sock,"I got your message",18)) < 0) {
	   close(sock);
       error("ERROR writing to socket");
   }
}