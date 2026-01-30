#ifndef _SERVER_H
#define _SERVER_H

int send_all(int socket_fd, const void *buf, size_t buf_length, int flags);

void run_server(void);

#endif // _SERVER_H
