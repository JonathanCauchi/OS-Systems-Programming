#include <time.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"
#include "proto.h"

struct cmd_info{
  enum cmd_type type;
  const char * name;
  const char * usage;
  const int argc[2];
  const int extract_host;
} cmd_infos[CMD_COUNT] = {
  {RUN,     "run",     "run 'job-description'", {2,2}, 1},
  {SUBMIT,  "submit",  "submit 'job-description' <now | data time>", {3, 5}, 1},
  {KILL,    "kill",    "kill job-ID <soft | hard | nice [grace-period]", {3,4}, 0},
  {STATUS,  "status",  "status",  {1,1}, 0},
  {CHDIR,   "chdir",   "chdir 'remote-host:remote-directory'", {2,2}, 1},
  {COPY,    "copy",    "copy <source-file target file> | <source-file ... target-directory>", {3, 10000}, 0},
  {GETID,   "getid",   "getid CMD", {3, 6}, 0},
  {STATE,   "state",   "state job-ID <SCHEDULED|RUNNING|TERMINATED>", {3, 3}, 0}
};

int send_cmd(const int fd, struct command * cmd){
  int i;

  const int val = htonl(cmd->argc);
  if(write(fd, &val, sizeof(int)) != sizeof(int)){
    perror("write");
    return -1; //no bytes were sent
  }

  if(write_msg(fd, cmd->hostname) < 0){
    return -1;
  }

  //send argv
  for(i=0; i < cmd->argc; i++){
    if(write_msg(fd, cmd->argv[i]) < 0){
      return -1;
    }
  }

  return 0;
};

int recv_cmd(const int fd, struct command * cmd){

  if(read(fd, &cmd->argc, sizeof(int)) != sizeof(int)){
    perror("read");
    return -1;
  }
  cmd->argc = ntohl(cmd->argc);
  cmd->hostname = read_msg(fd);

  cmd->argv = (char**) malloc(sizeof(char*)*(cmd->argc + 1));
  if(cmd->argv == NULL){
    perror("malloc");
    return -1;
  }

  int i;
  for(i=0; i < cmd->argc; i++){
    cmd->argv[i] = read_msg(fd);
  }

  return 0;
};

int extract_host(struct command * cmd){

  if(cmd->argc < 2){
    fprintf(stderr, "Error: Missing host/job-description argument\n");
    return -1;
  }

  int cmdlen = strlen(cmd->argv[1]);
  char * host = strtok(cmd->argv[1], ":");
  if(host == NULL){
    fprintf(stderr, "Error: Missing host\n");
    return -1;
  }

  int len = strlen(host);
  if((len + 1) < cmdlen){
    char * arg = &host[len+1];
    cmd->hostname = strdup(host);
    memmove(cmd->argv[1], arg, cmdlen - len);
  }else{
    fprintf(stderr, "Error: Missing argument after host\n");
    return -1;
  }
  return 0;
}

static int is_job_id(const char * str){
  const int len = strlen(str);
  int i;
  for(i=0; i < len; i++){
    if(isdigit(str[i]) == 0)
      return -1;
  }
  return 0;
}

int cmd_validate(struct command * cmd, int without_host){

  if(cmd->argc < 1){
    fprintf(stderr, "Usage: %s <run|submit | kill | status | copy | chdir\n", cmd->argv[0]);
    return -1;
  }

  int i;
  cmd->type = CMD_COUNT;
  for(i=0; i < CMD_COUNT; i++){
    if(strcmp(cmd->argv[0], cmd_infos[i].name) == 0){
      if( (cmd->argc < cmd_infos[i].argc[0]) ||
          (cmd->argc > cmd_infos[i].argc[1]) ){
            fprintf(stderr, "Usage: %s\n", cmd_infos[i].usage);
            return -1;
          }

      cmd->type = cmd_infos[i].type;

      if( (without_host == 0) &&
          cmd_infos[i].extract_host){
        if(extract_host(cmd) < 0){
          fprintf(stderr, "Error: Failed to extract host\n");
          return -1;
        }
      }
      break;
    }
  }

  if(cmd->type == CMD_COUNT){
    fprintf(stderr, "Error: Invalid command %s\n", cmd->argv[0]);
    return -1;

  }else if(cmd->type == GETID){
    //drop getid prefix from command
    free(cmd->argv[0]);
    free(cmd->hostname);
    cmd->hostname = cmd->argv[1];
    int i;
    for(i=2; i < cmd->argc; i++){
      cmd->argv[i-2] = cmd->argv[i];
    }
    cmd->argc-=2;

    int rc = cmd_validate(cmd, 1); //revalidate the command
    cmd->type = GETID;
    return rc;

  }else if( (cmd->type == SUBMIT) &&
            (strcmp(cmd->argv[cmd->argc-1], "now") == 0)  ){
      time_t t = time(NULL);
      struct tm * tmp = localtime(&t);
      if (tmp == NULL) {
        perror("localtime");
        return -1;
      }
      free(cmd->argv[cmd->argc-1]);

      cmd->argc++;
      cmd->argv = realloc(cmd->argv, sizeof(char*)*cmd->argc);
      cmd->argv[cmd->argc-2] = malloc(10+1);  //date
      cmd->argv[cmd->argc-1] = malloc(8+1);   //time

      if( (strftime(cmd->argv[cmd->argc-2], 11, "%m/%d/%Y", tmp) == 0) ||
          (strftime(cmd->argv[cmd->argc-1], 9, "%T", tmp) == 0) ){
        perror("strftime");
        return -1;
      }
  }else if(cmd->type == KILL){
    //check job-id
    if(is_job_id(cmd->argv[1]) < 0){
      fprintf(stderr, "Error: Invalid job-ID\n");
      return -1;
    }

    //check mode
    const char * modes[3] = {"soft", "hard", "nice"};
    for(i=0; i < 3; i++){
      if(strcmp(cmd->argv[2], modes[i]) == 0)
        break;
    }
    if(i == 3){
      fprintf(stderr, "Error: Invalid mode %s\n", cmd->argv[2]);
      return -1;
    }
  }else if(cmd->type == STATE){

    if(is_job_id(cmd->argv[1]) < 0){
      fprintf(stderr, "Error: Invalid job-ID\n");
      return -1;
    }

    const char * job_stat_label[3] = {"SCHEDULED", "RUNNING", "TERMINATED"};
    for(i=0; i < 3; i++){
      if(strcmp(cmd->argv[2], job_stat_label[i]) == 0)
        break;
    }

    if(i == 3){
      fprintf(stderr, "Error: Invalid state %s\n", cmd->argv[2]);
      return -1;
    }
  }

  return 0;
}

void free_cmd(struct command * cmd){
  int i;
  for(i=0; i < cmd->argc; i++){
    free(cmd->argv[i]);
  }
  free(cmd->argv);
  free(cmd->hostname);
  free(cmd);
}

struct command * new_cmd(){
  struct command *cmd = (struct command *) malloc(sizeof(struct command));
  cmd->argc = 0;
  cmd->hostname = NULL;
  cmd->argv = NULL;
  return cmd;
}

char* cmd_common(struct command * cmd){
  const int jd = connect_to(cmd->hostname);  //returns a socket descriptor for job
  if(jd == -1){
    fprintf(stderr, "Error: Failed to connect to %s\n", cmd->hostname);
    return NULL;
  }

  if(send_cmd(jd, cmd) == -1){
    fprintf(stderr, "Error: Failed to send command to %s\n", cmd->hostname);
    return NULL;
  }

  char * reply = read_msg(jd);  //read job id from server
  close(jd);

  if(reply == NULL)
    fprintf(stderr, "Error: Failed to get reply from %s\n", cmd->hostname);

  return reply;
}
