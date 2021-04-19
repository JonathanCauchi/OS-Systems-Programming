#define _GNU_SOURCE
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <wordexp.h>
#include <pthread.h>

#include "rex.h"
#include "proto.h"
#include "cmd.h"

static int listenfd = -1;
static int run_server = 1;
static char * rexd_leader = NULL;

static const char * job_stat_label[3] = {"SCHEDULED", "RUNNING", "TERMINATED"};

struct joblist{
  struct job ** jobs;
  unsigned int count;
  unsigned int size;
} joblist = {NULL, 0, 0};
static pthread_rwlock_t joblist_lock = PTHREAD_RWLOCK_INITIALIZER;

static int server_skt(const int port){

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));

  int skt = socket(AF_INET, SOCK_STREAM, 0);
  if(skt == -1){
    perror("socket");
    return -1;
  }

  int v = 1;
  if (setsockopt(skt, SOL_SOCKET,SO_REUSEADDR, &v, sizeof(int)) == -1) {
	   perror("setsockopt");
	    return -1;
  }

  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port        = htons(port);

  if(bind(skt, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) ==-1){
    perror("bind");
    return -1;
  }

  if (listen(skt, 10) == -1) {
    perror("Failed to listen");
	  return -1;
  }
  return skt;
}

static void free_job(struct job * j){
  free(j->job_desc);
  free(j->hostname);
  free(j);
}

static int exit_cleanup(const int code){

  if(listenfd > 0){
    shutdown(listenfd, SHUT_RDWR);
    close(listenfd);
  }

  int i;
  for(i=0; i < joblist.count; i++){
    free_job(joblist.jobs[i]);
  }
  free(joblist.jobs);
  if(rexd_leader)
    free(rexd_leader);

  exit(code);
}

static struct job * get_job_by_id(const int id){
  int i;
  struct job * j = NULL;

  pthread_rwlock_rdlock(&joblist_lock);
  for(i=0; i < joblist.count; i++){
    if(joblist.jobs[i]->id == id){
      j = joblist.jobs[i];
      break;
    }
  }
  pthread_rwlock_unlock(&joblist_lock);
  return j;
}

//add to list of jobs
static int joblist_add(struct job * j){

  pthread_rwlock_wrlock(&joblist_lock);

  if(joblist.count == joblist.size){
    joblist.size += 10;
    struct job ** temp = realloc(joblist.jobs, sizeof(struct job*)*joblist.size);
    if(temp == NULL){
      perror("realloc");
      pthread_rwlock_unlock(&joblist_lock);
      return -1;
    }
    joblist.jobs = temp;
  }

  //prepend to joblist
  joblist.jobs[joblist.count++] = j;

  pthread_rwlock_unlock(&joblist_lock);
  return 0;
}

static time_t convert_datetime(const char * date, const char * time){
  char buf[20];
  struct tm tm;
  bzero(&tm, sizeof(struct tm));

  snprintf(buf, sizeof(buf), "%s %s", date, time);
  if(strptime(buf, "%m/%d/%Y %T", &tm) != &buf[19]){
    perror("strptime");
    return 0;
  }

  time_t t = timelocal(&tm);
  if(t == ((time_t) -1)){
    perror("mktime");
    return 0;
  }
  return t;
}

static char ** convert_job_description(const char * job_desc, int *argc){
  wordexp_t p;
  int i;

  wordexp(job_desc, &p, 0);

  *argc = p.we_wordc;
  char ** argv = (char**) malloc(sizeof(char*) * (*argc+1));

  for (i = 0; i < p.we_wordc; i++){
    argv[i] = strdup(p.we_wordv[i]);
  }
  wordfree(&p);

  argv[*argc] = NULL;

  return argv;
}

static int get_jobid(){
  static unsigned int job_id = 1;
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&lock);
  int myid = job_id++;
  pthread_mutex_unlock(&lock);
  return myid;
}

static int cmd_getid(const struct command * cmd){

  struct command * req = new_cmd();
  req->type = GETID;
  req->hostname = strdup(rexd_leader);  //command goes to leader
  req->argc = cmd->argc + 2;  //getid hostname cmd args...
  req->argv = (char**) malloc(sizeof(char*)*req->argc);
  req->argv[0] = strdup("getid");
  req->argv[1] = (char*) malloc(HOST_NAME_MAX);
  gethostname(req->argv[1], HOST_NAME_MAX);

  int i;
  for(i=2; i < req->argc; i++)
    req->argv[i] = strdup(cmd->argv[i-2]);

  char * reply = cmd_common(req);

  int id;
  if(reply == NULL){
    id = -1;
    fprintf(stderr, "Error: Couldn't get ID for job\n");
  }else{
    id = atoi(reply);
    free(reply);
    printf("Got job id %d\n", id);
  }

  return id;
}

//called when a job changes status
static int cmd_state(const int jobid, const enum job_state state){

  struct command * req = new_cmd();
  req->type = STATE;
  req->hostname = strdup(rexd_leader);
  req->argc = 3;
  req->argv = (char**) malloc(sizeof(char*)*req->argc);
  req->argv[0] = strdup("state");

  req->argv[1] = (char*) malloc(10);
  if(req->argv[1] == NULL)
    return -1;
  snprintf(req->argv[1], 10, "%d", jobid);

  req->argv[2] = strdup(job_stat_label[state]);

  char * reply = cmd_common(req);
  if(reply == NULL){
    return -1;
  }else{
    free(reply);
  }

  return 0;
}

static int job_change_state(struct job * j, enum job_state state){

  if(rexd_leader == NULL){
    pthread_rwlock_wrlock(&joblist_lock);
    j->state = state;
    pthread_rwlock_unlock(&joblist_lock);
  }else{
    cmd_state(j->id, state);
  }
  return 0;
}

static int srv_state(const int clientfd, struct command * cmd){

  int id = atoi(cmd->argv[1]);
  struct job * j = get_job_by_id(id);
  if(j == NULL){
    fprintf(stderr, "Error: Job %d not found\n", id);
    return -1;
  }

  enum job_state new_state = SCHEDULED;
  switch(cmd->argv[2][0]){
    case 'R': new_state = RUNNING;    break;
    case 'S': new_state = SCHEDULED;  break;
    case 'T': new_state = TERMINATED; break;
    default:  break;
  }

  job_change_state(j, new_state);

  return write_msg(clientfd, "ok");
}

static struct job* new_job(const int fd, struct command * cmd){

  struct job * j = (struct job*) malloc(sizeof(struct job));
  if(j == NULL){
    perror("malloc");
    return NULL;
  }

  if(rexd_leader == NULL){  //if we are the leader
    j->id = get_jobid();
  }else{
    j->id = cmd_getid(cmd);  //ask leader for an id
  }

  j->fd = fd;
  j->pid = 0;
  j->time = 0;
  j->state = SCHEDULED;

  j->job_desc = strdup(cmd->argv[1]);
  j->hostname = strdup(cmd->hostname);

  if(cmd->type == SUBMIT){
    j->time = convert_datetime(cmd->argv[2], cmd->argv[3]);
    if(j->time == 0){
      free_job(j);
      return NULL;
    }
    j->type = BATCH;
  }else{
    j->type = INTERACTIVE;
    j->time = time(NULL);
  }

  joblist_add(j);

  return j;
}

static int srv_run(struct job * j){
  char buf[20];

  int     argc = 0;
  char ** argv = NULL;

  const time_t wait_time = time(NULL) - j->time;
  if(wait_time > 0){
    //printf("Waiting %li seconds for job %d\n", time(NULL) - j->time, j->id);
    sleep(wait_time);
  }


  job_change_state(j, RUNNING);    //inform leade job is running

  pid_t pid = fork();
  switch(pid){
    case -1:
      perror("fork");
      return 1;
      break;

    case 0:
      close(listenfd);
      argv = convert_job_description(j->job_desc, &argc);

      if(j->type == BATCH){
        snprintf(buf, sizeof(buf), "%d", j->id);
        write_msg(j->fd, buf);  //send job it to client

        //Redirect our/err to files for batch jobs
        snprintf(buf, sizeof(buf), "%d.out", j->id);
        stdout = freopen(buf, "w", stdout);
        snprintf(buf, sizeof(buf), "%d.err", j->id);
        stderr = freopen(buf, "w", stderr);

      }else{
        if( (dup2(j->fd, 0) == -1) ||
            (dup2(j->fd, 1) == -1) ||
            (dup2(j->fd, 2) == -1) ){
            perror("dup2");
            exit(EXIT_FAILURE);
        }
      }
      close(j->fd);


      if(isalpha(argv[0][0])){
        execvp(argv[0], argv);
      }else{
        execv(argv[0], argv);
      }

      perror("execv");
      exit(EXIT_FAILURE);
      break;

    default:
      j->pid = pid;
      close(j->fd);
      break;
  }

  return 0;
}

static int srv_kill(const int jobid, enum kill_mode km, const int grace_period){
  struct job * j = get_job_by_id(jobid);
  if(j == NULL)
    return EXIT_FAILURE;

  switch(km){
    case NICE:
      sleep(grace_period);
    case SOFT:
      kill(j->pid, SIGTERM);
      break;
    case HARD:
      kill(j->pid, SIGKILL);
      break;
  }

  return EXIT_SUCCESS;
}

static int srv_status(const int clientfd){

  char row[74], datebuf[11], timebuf[9];
  const char job_type_label[2] = "IB";

  //we use {send,recv}_cmd, where cmd->argv[] holds a row of status output
  struct command *cmd = new_cmd();

  cmd->hostname = malloc(HOST_NAME_MAX);
  gethostname(cmd->hostname, HOST_NAME_MAX);

  snprintf(row, sizeof(row), "%3s %18s %14s %4s %10s %10s %8s",
              "Job", "Host", "Command", "Type", "Status", "Date", "Time");

  pthread_rwlock_rdlock(&joblist_lock);

  cmd->argv = (char**) malloc(sizeof(char*) * (joblist.count+1));
  cmd->argv[cmd->argc++] = strdup(row);

  int i;
  for(i=0; i < joblist.count; i++){
    struct job * j = joblist.jobs[i];

    struct tm * tmp = localtime(&j->time);
    if (tmp == NULL) {
      perror("localtime");
      break;
    }

    if( (strftime(datebuf, 11, "%m/%d/%Y", tmp) == 0) ||
        (strftime(timebuf, 9, "%T", tmp) == 0) ){
      perror("strftime");
      break;
    }

    snprintf(row, sizeof(row), "%3d %18s %14s %4c %10s %10s %8s",
      j->id, j->hostname, j->job_desc, job_type_label[j->type], job_stat_label[j->state], datebuf, timebuf);

    cmd->argv[cmd->argc++] = strdup(row);
  }
  pthread_rwlock_unlock(&joblist_lock);

  send_cmd(clientfd, cmd);
  free_cmd(cmd);

  return 0;
}

static int srv_copy(const int clientfd, struct command * cmd){
  int rc;
  const char * mode = cmd->argv[1];

  //if client wants to read, we have to write
        if(strcmp(mode, "readfile") == 0){    rc = send_file(clientfd, cmd->argv[2]);
  }else if(strcmp(mode, "readdir") == 0){     rc = send_dir(clientfd,  cmd->argv[2]);
  }else if(strcmp(mode, "writefile") == 0){   rc = recv_file(clientfd, cmd->argv[2]);
  }else if(strcmp(mode, "writedir") == 0){    rc = recv_dir(clientfd,  cmd->argv[2]);
  }else{
    write_msg(clientfd, "Error: Unknown copy mode\n");
    rc = -1;
  }
  return rc;
}

static int srv_chdir(const int fd, const char * remote_dir){
  struct stat st;
  if(stat(remote_dir, &st) == -1){
    write_msg(fd, "chdir: invalid directory\n");
  }else if(chdir(remote_dir) == -1){
    write_msg(fd, "chdir: failed to change directory\n");
    perror("chdir");
  }else{
    printf("CHDIR: %s\n", remote_dir);
    write_msg(fd, "chdir: directory changed\n");
  }
  return 0;
}

static int srv_getid(const int fd, struct command * cmd){

  struct job * j = new_job(-1, cmd);
  if(j == NULL)
    return -1;

  char buf[10];
  snprintf(buf, sizeof(buf), "%i", j->id);
  return write_msg(fd, buf);
}

static void* client_handler(void* arg){

  int clientfd = *(int*) arg;
  free(arg);

  struct job * j = NULL;
  struct command *cmd = new_cmd();

  if( (recv_cmd(clientfd, cmd) < 0) ||
      (cmd_validate(cmd, 1) < 0) ){
    write_msg(clientfd, "Error: Invalid command\n");
    free(cmd);
    pthread_exit(NULL);
  }

  int jobid = 0, grace_period = 1;
  enum kill_mode km = SOFT;

  int rc = 0;
  switch(cmd->type){
    case RUN:
    case SUBMIT:
      j = new_job(clientfd, cmd);
      rc = srv_run(j);
      break;

    case STATUS:  rc = srv_status(clientfd);              break;
    case CHDIR:   rc = srv_chdir(clientfd, cmd->argv[1]); break;
    case COPY:    rc = srv_copy(clientfd, cmd);           break;
    case GETID:   rc = srv_getid(clientfd, cmd);          break;
    case STATE:   rc = srv_state(clientfd, cmd);           break;

    case KILL:
      if(strcmp(cmd->argv[3], "nice") == 0){
        km = NICE;
        if(cmd->argc == 5)
          grace_period = atoi(cmd->argv[4]);
      }else if(strcmp(cmd->argv[3], "soft") == 0){
        km = SOFT;
      }else if(strcmp(cmd->argv[3], "hard") == 0){
        km = HARD;
      }
      jobid = atoi(cmd->argv[1]);
      rc = srv_kill(jobid, km, grace_period);
      break;

    case CMD_COUNT: break;
  }
  free(cmd);

  shutdown(clientfd, SHUT_RDWR);
  close(clientfd);


  pthread_exit(NULL);
}

static int process_clients(){
  struct sockaddr_in client_addr;
  socklen_t socksize = sizeof(struct sockaddr_in);

  while (run_server) {
     const int clientfd = accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
     if(clientfd < 0){
       if(errno != EINTR){
         perror("accept");
         break;
       }else{
         continue;  //restart accept on interrupt
       }
     }

     pthread_t thr;

     int * arg = malloc(sizeof(int));
     if(arg == NULL){
       perror("malloc");
       break;
     }

     *arg = clientfd;
     if(pthread_create(&thr, NULL, client_handler, (void*)arg) != 0){
       perror("pthread_create");
       break;
     }
     pthread_detach(thr);
  }

  return EXIT_SUCCESS;
}

static void sig_handler(int sig, siginfo_t * si, void* unused){
  int i;
  struct job * j = NULL;

  switch(sig){
    case SIGINT:
    case SIGTERM:
      run_server = 0;
      break;
    case SIGCHLD:
      //go through each child and release terminated
      pthread_rwlock_wrlock(&joblist_lock);

      for(i=0; i < joblist.count; i++){
        int wstatus;
        if(waitpid(joblist.jobs[i]->pid, &wstatus, WNOHANG | WUNTRACED | WCONTINUED) == -1){
          continue;
        }else{
          if (WIFEXITED(wstatus)) {           printf("exited, status=%d\n", WEXITSTATUS(wstatus));
          } else if (WIFSIGNALED(wstatus)) {  printf("killed by signal %d\n", WTERMSIG(wstatus));
          } else if (WIFSTOPPED(wstatus)) {   printf("stopped by signal %d\n", WSTOPSIG(wstatus));
          } else if (WIFCONTINUED(wstatus)) { printf("continued\n");
          }
          j = joblist.jobs[i];
          break;
        }
      }
      pthread_rwlock_unlock(&joblist_lock);

      if(j)
        job_change_state(j, TERMINATED);
      break;
  }
}

static int parse_args(const int argc, char * const argv[]){

  if(argc == 2){  //second parameter tells service, who is the master
    rexd_leader = strdup(argv[1]);
    printf("Electing as leader: %s\n", rexd_leader);
  }else{
    rexd_leader = NULL;
    printf("Acting as Rexd leader\n");
  }
  return 0;
}

static int init_service(){

  //services run as daemons
  //TODO: uncomment after testing
  /*if(daemon(0, 0) == -1){
    perror("daemon");
    return -1;
  }*/

  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sig_handler;
  if( (sigaction(SIGTERM, &sa, NULL) == -1) ||
      (sigaction(SIGINT, &sa, NULL) == -1) ||
      (sigaction(SIGCHLD, &sa, NULL) == -1) ){
    perror("sigaction");
    return -1;
  }

  if((listenfd = server_skt(REX_PORT)) == -1)
      return -1;

  return 0;
}

int main(const int argc, char * const argv[]){

  parse_args(argc, argv);

  if(init_service() < 0)
    return EXIT_FAILURE;

  process_clients();

  exit_cleanup(EXIT_SUCCESS);
}
