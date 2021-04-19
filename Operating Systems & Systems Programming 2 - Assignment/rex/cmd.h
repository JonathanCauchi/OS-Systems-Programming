enum cmd_type {RUN=0, SUBMIT, KILL, STATUS, CHDIR, COPY, GETID, STATE, CMD_COUNT};
struct command{
  enum cmd_type type;
  char * hostname;
  char ** argv;
  int     argc;
};

int send_cmd(const int fd, struct command * cmd);
int recv_cmd(const int fd, struct command * cmd);

int cmd_validate(struct command * cmd, const int without_host);

struct command * new_cmd();
void free_cmd(struct command * cmd);

char* cmd_common(struct command * cmd);
