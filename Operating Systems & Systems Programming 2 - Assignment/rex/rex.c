#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "rex.h"
#include "proto.h"
#include "cmd.h"

static char * rexd_leader = NULL;

static int parse_cmd(struct command * cmd, const int argc, char * const argv[]){

  int i, opti=1;

  if( (argc > 3) &&  //second parameter tells who is the rexd leader
      (strcmp(argv[1], "-l") == 0) ){
      rexd_leader = strdup(argv[2]);
      printf("Using as leader: %s\n", rexd_leader);
      opti = 3;
  }else{
    rexd_leader = strdup("localhost");
    printf("Using as leader: %s\n", rexd_leader);
  }

  cmd->argc = argc - opti;
  cmd->argv = (char**) malloc(sizeof(char*)*cmd->argc);
  cmd->hostname = NULL;

  for(i=0; i < cmd->argc; i++)
    cmd->argv[i] = strdup(argv[i+opti]); //copy job-description

  if(cmd_validate(cmd, 0) < 0)
    return -1;

  if(cmd->hostname == NULL){
    cmd->hostname = strdup(rexd_leader);
  }

  return 0;
}

static int cmd_run(struct command * cmd){

  const int jd = connect_to(cmd->hostname);  //returns a socket descriptor for job
  if(jd == -1){
    fprintf(stderr, "Error: Failed to connect to %s\n", cmd->hostname);
    return EXIT_FAILURE;
  }

  if(send_cmd(jd, cmd) < 0)
    return EXIT_FAILURE;

  int flag = 1;
  char buf[100];
  while(flag){

    fd_set rfds;
    FD_ZERO(&rfds);
    if(!feof(stdin))
      FD_SET(0, &rfds);
    FD_SET(jd, &rfds);

    if (select(jd + 1, &rfds, NULL, NULL, NULL) == -1){
      perror("select");
      break;
    }

    if(FD_ISSET(0, &rfds)){ //data on stdin
      const int len = read(0, buf, sizeof(buf));
      switch(len){
        case -1:
          perror("read");
        case 0: //EOF on stdin
          shutdown(jd, SHUT_WR);  //close socket for writing
          break;
        default:
          if(write(jd, buf, len) != len){
            perror("write");
          }
          break;
      }

    }else if(FD_ISSET(jd, &rfds)){  //data on socket
      int len = read(jd, buf, sizeof(buf));
      if(len == 0){ //socket closed
        flag = 0; //stop the loop
      }else{
        write(1, buf, len);
      }
    }
  }//end while

  close(jd);

  return EXIT_SUCCESS;
}

static int cmd_copy_real(const char * host, const char * mode, const char * remote, const char * local){

  const int jd = connect_to(host);  //returns a socket descriptor for job
  if(jd == -1){
    fprintf(stderr, "Error: Failed to connect to %s\n", host);
    return -1;
  }

  struct command * cmd = new_cmd();
  cmd->type = COPY;
  cmd->argc = 3;
  cmd->hostname = strdup(rexd_leader);
  cmd->argv = (char**) malloc(sizeof(char*)*cmd->argc);
  cmd->argv[0] = strdup("copy");
  cmd->argv[1] = strdup(mode);
  cmd->argv[2] = strdup(remote);

  if(send_cmd(jd, cmd) == -1){      //tell which file
    fprintf(stderr, "Error: Failed to send command to %s\n", host);
    free_cmd(cmd);
    return -1;
  }
  free_cmd(cmd);

  int rc;
       if(strcmp(mode, "readfile") == 0){     rc = recv_file (jd, local);  //read host:target to source
  }else if(strcmp(mode, "readdir") == 0){     rc = recv_dir  (jd, local);
  }else if(strcmp(mode, "writefile") == 0){   rc = send_file(jd, local);  //send source to host:target
  }else if(strcmp(mode, "writedir") == 0){    rc = send_dir (jd, local);
  }else{
    write_msg(jd, "Error: Unknown copy mode\n");
    rc = -1;
  }

  return rc;
}

static int cmd_copy(struct command * cmd){

  const int num_sources = cmd->argc - 1 - 1; //-1 for copy cmd, -1 for target

  printf("Copying %d files...\n", num_sources);

  if(num_sources == 1){ //source_file <-> target_file
    if(strchr(cmd->argv[1], ':')){  //source is server - read mode

      char * host = strtok(cmd->argv[1], ":");
      char * remote = strtok(NULL, ":");
      char * local = cmd->argv[2];

      printf("%s:%s -> %s\n", host, remote, local);

      //what is source is a dir
      int len = strlen(remote);
      if(remote[len-1] == '/'){
        return cmd_copy_real(host, "readdir", remote, local);
      }else{

        struct stat st;
        if(stat(local, &st) == -1){
          //perror("stat");
          return cmd_copy_real(host, "readfile", remote, local);
        }

        int rc = 0;
        if(S_ISDIR(st.st_mode)){ //local is a local directory
          //biuld filename for local dir
          const int len = strlen(local) + 1 + strlen(remote) + 1;
          char * localpath = (char*) malloc(len);
          snprintf(localpath, len, "%s/%s", local, remote);
          rc = cmd_copy_real(host, "readfile", remote, localpath);
        }else{
          rc = cmd_copy_real(host, "readfile", remote, local);
        }
        return rc;
      }

    }else if(strchr(cmd->argv[cmd->argc-1], ':')){  //target is server - write mode

      char * host = strtok(cmd->argv[cmd->argc-1], ":");
      char * remote = strtok(NULL, ":");
      char * local = cmd->argv[1];

      printf("%s -> %s:%s\n", local, host, remote);

      struct stat st;
      if(stat(local, &st) == -1){
        perror("stat");
        return -1;
      }

      if(S_ISDIR(st.st_mode)){ //local is a local directory
        return cmd_copy_real(host, "writedir", remote, local);  //dir -> target
      }else if(S_ISREG(st.st_mode)){ //local is a file
        return cmd_copy_real(host, "writefile", remote, local);   //file -> target
      }else{
        fprintf(stderr, "Error: Source is not file/dir\n");
        return -1;
      }

    }else{
      fprintf(stderr, "Error: No host\n");
      return -1;
    }
  }else{  //source_file ... <-> target_directory

    if(strchr(cmd->argv[1], ':')){  //source is server
      const char * local = cmd->argv[cmd->argc-1]; //target must be a loca dir

      struct stat st;
      if(stat(local, &st) == -1){
        perror("stat");
        return -1;
      }

      if(S_ISDIR(st.st_mode) == 0){
        fprintf(stderr, "Error: Target must be a directory\n");
        return -1;
      }

      int i;
      for(i=1; i <= num_sources; i++){ //for each source
        char * host = strtok(cmd->argv[i], ":");
        char * remote = strtok(NULL, ":");

        //biuld filename for local dir
        const int len = strlen(local) + 1 + strlen(remote) + 1;
        char * localpath = (char*) malloc(len);
        snprintf(localpath, len, "%s/%s", local, remote);

        printf("%s:%s -> %s\n", host, remote, localpath);

        if(cmd_copy_real(host, "readfile", remote, localpath) < 0){
          free(localpath);
          return -1;
        }
        free(localpath);
      }

    }else if(strchr(cmd->argv[cmd->argc-1], ':')){  //target is server
      char * host = strtok(cmd->argv[cmd->argc-1], ":");
      char * remote = strtok(NULL, ":");  //taget must be a directory on server

      int i;
      for(i=1; i <= num_sources; i++){
        char * local = cmd->argv[i];
        const int len = strlen(local) + 1 + strlen(remote) + 1;
        char * remotepath = (char*) malloc(len);
        snprintf(remotepath, len, "%s/%s", remote, local);

        printf("%s -> %s:%s\n", local, host, remotepath);

        if(cmd_copy_real(host, "writefile", remotepath, local) < 0){
          free(remotepath);
          return -1;
        }
        free(remotepath);
      }
    }else{
      fprintf(stderr, "Error: Source is not file/dir\n");
      return -1;
    }
  }

  return EXIT_SUCCESS;
}

static int cmd_status(struct command * cmd){
  const int jd = connect_to(cmd->hostname);  //returns a socket descriptor for job
  if(jd == -1){
    fprintf(stderr, "Error: Failed to connect to %s\n", cmd->hostname);
    return EXIT_FAILURE;
  }

  if(send_cmd(jd, cmd) == -1){
    fprintf(stderr, "Error: Failed to send command to %s\n", cmd->hostname);
    return EXIT_FAILURE;
  }
  //read status rows from reply

  struct command * reply = new_cmd();
  if(recv_cmd(jd, reply) == -1){
    return EXIT_FAILURE;
  }
  close(jd);

  int i;
  for(i=0; i < reply->argc; i++){
    printf("%s\n", reply->argv[i]);
  }
  free_cmd(reply);

  return EXIT_SUCCESS;
}

static int execute_cmd(struct command * cmd){
  int rc = EXIT_FAILURE;
  char * reply = NULL;

  switch(cmd->type){
    case RUN:     rc = cmd_run(cmd);    break;
    case SUBMIT:
      reply = cmd_common(cmd);
      if(reply){
        printf("Job \"%s:%s\" submitted; unique ID is %s\n", cmd->hostname, cmd->argv[0], reply);
        rc = EXIT_SUCCESS;
      }else{
        rc = EXIT_FAILURE;
      }
      break;

    case COPY:    rc = cmd_copy(cmd);   break;

    case CHDIR:
    case KILL:
       reply = cmd_common(cmd);
       if(reply){
         printf("%s", reply);
         rc = EXIT_SUCCESS;
      }else{
        rc = EXIT_SUCCESS;
      }
      break;

    case STATUS:
      rc = cmd_status(cmd);
      break;

    //server only commands
    case GETID:
    case STATE:
    case CMD_COUNT:
      break;
  }

  if(reply){
    free(reply);
  }
  return rc;
}

static void exit_cleanup(struct command * cmd, const int code){
  free_cmd(cmd);
  free(rexd_leader);
  exit(code);
}

int main(const int argc, char * const argv[]){
  int rc = EXIT_SUCCESS;

  struct command *cmd = new_cmd();

  if( (parse_cmd(cmd, argc, argv) < 0) ||
      (execute_cmd(cmd) == EXIT_FAILURE) ){
    rc = EXIT_FAILURE;
  }

  exit_cleanup(cmd, rc);
}
