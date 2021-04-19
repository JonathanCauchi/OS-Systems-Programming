#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

// Socket path
char *socket_path = "\0hidden";

int main(int argc, char *argv[]) 
{
	// Declare variables
	struct sockaddr_un addr;
	char buf[100];
	int fd, cl,rc;

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
		unlink(socket_path);
	}

	// Bind to address
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	// Listen
	if (listen(fd, 5) == -1) {
		perror("listen error");
		exit(-1);
	}

	// Execute forever
	while (1) {
		
		// Accept new client
		if ( (cl = accept(fd, NULL, NULL)) == -1) {
		  perror("accept error");
		  continue;
		}

		// Read from client
		while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
		   printf("read %u bytes: %.*s\n", rc, rc, buf);
		}
		
		if (rc == -1) {
		  perror("read");
		  exit(-1);
		}
		else if (rc == 0) {
		  printf("EOF\n");
		  close(cl);
		}
	}


	return 0;
}
