#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "proto.h"

char* read_msg(const int fd){
  int msg_len;

  if(read(fd, &msg_len, sizeof(int)) != sizeof(int)){
    perror("read");
    return NULL;
  }

  msg_len = ntohl(msg_len);

  char * msg = (char*) malloc(sizeof(char)*(msg_len+1));
  if(msg == NULL){
    perror("malloc");
    return NULL;
  }
  msg[msg_len] = '\0';

  int offset = 0;
  while(offset < msg_len){
    int bytes = read(fd, &msg[offset], msg_len - offset);
    if(bytes <= 0){
      perror("read");
      return msg;
    }

    offset += bytes;
  }

  return msg;
};

int write_msg(const int fd, const char *buf){
  const int len = strlen(buf);
  const int val = htonl(len);
  if(write(fd, &val, sizeof(int)) != sizeof(int)){
    perror("write");
    return -1; //no bytes were sent
  }

  int offset = 0;
  while(offset < len){
    int bytes = write(fd, &buf[offset], len - offset);
    if(bytes <= 0){
      perror("write");
      return -1;
    }

    offset += bytes;
  }
  return offset;
};

static int transfer_data(const int in, const int out, const int size){
  char buf[100];
  int transfered = 0;
  while(transfered < size){
    int bytes = read(in, buf, sizeof(buf));
    if(write(out, buf, bytes) != bytes){
      perror("write");
      return -1;
    }
    transfered += bytes;
  }
  return transfered;
}

int recv_file(const int sd, const char * filename){

  int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IWUSR | S_IRUSR);
  if(fd == -1){
    perror("open");
    return -1;
  }

  int len;
  if(read(sd, &len, sizeof(int)) != sizeof(int)){
    perror("read");
    return -1;
  }
  len = ntohl(len);

  return transfer_data(sd, fd, len);
}

int send_file(const int sd, const char * filename){

  int fd = open(filename, O_RDONLY);
  if(fd == -1){
    perror("open");
    return -1;
  }

  struct stat st;
  if(stat(filename, &st) == -1){
    perror("stat");
    return -1;
  }

  int len = htonl(st.st_size);
  if(write(sd, &len, sizeof(int)) != sizeof(int)){
    perror("read");
    return -1;
  }

  return transfer_data(fd, sd, st.st_size);
}

int send_dir(const int sd, const char * dirname){

  DIR * ddir = opendir(dirname);
  if(ddir == NULL){
    perror("opendir");
    return -1;
  }

  char * fpath = malloc(PATH_MAX);
  if(fpath == NULL){
    perror("malloc");
    return -1;
  }
  int offset = snprintf(fpath, PATH_MAX-1, "%s/", dirname);

  struct stat st;
  struct dirent * dirp;

  while( (dirp = readdir(ddir)) != NULL ){
    if(stat(dirp->d_name, &st) == -1){
      perror("stat");
      break;
    }

    if((S_ISDIR(st.st_mode)) ){	//is not a directory
      continue;
    }

    strncpy(&fpath[offset], dirp->d_name, PATH_MAX-1-offset);

    if(write_msg(sd, fpath) < 0){      //tell which file
      fprintf(stderr, "Error: Failed to send filename\n");
      break;
    }
    const int len = send_file(sd, fpath);
    if(len < 0)
      break;

    printf("%d %s\n", len, fpath);
  }
  closedir(ddir);
  return 0;
}

int recv_dir(const int sd, const char * dirname){

  if(mkdir(dirname, S_IRWXU) == -1){
    if(errno != EEXIST){
      perror("mkdir");
      return -1;
    }
  }

  while(1){
    char * fpath = read_msg(sd); //read filename
    if(fpath == NULL)
      break;

    const int len = recv_file(sd, fpath);
    if(len < 0)
      break;

    printf("%d %s\n", len, fpath);
  }

  return 0;
}

int connect_to(const char * host){
  int sfd;
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;  /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;      /* For wildcard IP address */
  hints.ai_protocol = 0;            /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  char service[10];
  snprintf(service, sizeof(service), "%i", REX_PORT);

  int s = getaddrinfo(host, service, &hints, &result);
  if (s != 0) {
     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
     return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
           rp->ai_protocol);
    if (sfd == -1)
       continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
       break;                  /* Success */

    close(sfd);
  }

  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not bind\n");
    return -1;
  }

  freeaddrinfo(result);           /* No longer needed */
  return sfd;
}
