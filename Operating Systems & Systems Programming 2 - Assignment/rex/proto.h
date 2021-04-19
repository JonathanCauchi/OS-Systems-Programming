#define REX_PORT 20000

char* read_msg(const int sd);
int write_msg(const int sd, const char *buf);

int recv_file(const int sd, const char * filename);
int recv_dir(const int sd, const char * dirname);

int send_file(const int sd, const char * filename);
int send_dir(const int sd, const char * dirname);

int connect_to(const char * host);
