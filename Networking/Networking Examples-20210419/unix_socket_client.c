#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Socket path
char *socket_path = "\0hidden";

int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_un addr;
 	char buf[100];
	int fd,rc;

	// If path is passed in as argument, use it
	if (argc > 1) 
		socket_path=argv[1];

	// Create socket (note AF_UNIX)
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	// Set address structure
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	
	// Copy path to sun_path in address structure
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
	}

	// Connect 
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("connect error");
		exit(-1);
	}

	// Perform read/write
	while( (rc=read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
	if (write(fd, buf, rc) != rc) {
	  if (rc > 0) fprintf(stderr,"partial write");
	  else {
		perror("write error");
		exit(-1);
	  }
	}
	}

	return 0;
}