enum job_type {INTERACTIVE=0, BATCH};
enum job_state {SCHEDULED=0, RUNNING, TERMINATED};

enum kill_mode {SOFT=0, HARD, NICE};

struct job{
  int id;
  int fd;
  pid_t pid;
  time_t time;
  enum job_state state;
  enum job_type type;

  char * hostname;
  char * job_desc;
};
