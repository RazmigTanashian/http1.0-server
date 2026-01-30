#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stddef.h>
#include "server.h"
#include "http.h"
#include "utils.h"

#define PORT_NUMBER 8080

static void handle_client_connection(int client_sfd) {
	char recv_buf[256] = { '\0' };
	int r = recv(client_sfd, recv_buf, ARRAY_LENGTH(recv_buf), 0);
	if (r < 0) {
		perror("recv");
		exit(EXIT_FAILURE);
	}

	printf("\nIncoming request:\n");
	printf("%s", recv_buf);

	enum http_method m = http_retrieve_method(recv_buf, ARRAY_LENGTH(recv_buf));
	if (m == HTTP_METHOD_GET) {
		http_handle_request_get(client_sfd, recv_buf, ARRAY_LENGTH(recv_buf));
	} else if (m == HTTP_METHOD_POST) {
		http_handle_request_post(client_sfd, recv_buf, ARRAY_LENGTH(recv_buf));
	} else if (m == HTTP_METHOD_HEAD) {
		http_handle_request_head(client_sfd, recv_buf, ARRAY_LENGTH(recv_buf));
	} else {
		printf("Http1.0 request method received from the client was unrecognized!\n");
		http_handle_request_unrecognized(client_sfd);
	}

	printf("close()ing client sfd!\n");
	if (close(client_sfd) < 0) {
		perror("close");
		exit(EXIT_FAILURE);
	}
}

static void setup_server_socket(int *sock_fd) {
	struct sockaddr_in my_addr;
	int opt = 1;
	if ( (*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        if (setsockopt(*sock_fd,
                       SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                       &opt, sizeof(opt))) {
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }

        memset(&my_addr, 0, sizeof(my_addr));
        my_addr.sin_family = AF_INET;
        my_addr.sin_addr.s_addr = INADDR_ANY;
        my_addr.sin_port = htons(PORT_NUMBER);
        if (bind(*sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
                perror("bind");
                exit(EXIT_FAILURE);
        }

        if (listen(*sock_fd, 10) < 0) {
                perror("listen");
                exit(EXIT_FAILURE);
        }
}

int send_all(int socket_fd, const void *buf, size_t buf_length, int flags) {
	ssize_t num_bytes_sent = 0;
	const void *ptr = buf;

	while (buf_length > 0) {
		num_bytes_sent = send(socket_fd, ptr, buf_length, flags);

		if (num_bytes_sent == -1) {
			perror("send");
			return -1;
		}

		ptr += num_bytes_sent;
		buf_length -= num_bytes_sent;
	}

	return 0;
}

void run_server(void) {
	int server_sfd, client_sfd;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	pid_t pid;

	setup_server_socket(&server_sfd);
	if (server_sfd < 0) {
		perror("setup_server_socket");
		exit(EXIT_FAILURE);
	}

	printf("Waiting for client connections...\n");
	while(1) {
		if ((client_sfd = accept(server_sfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			perror("accept");
			continue;
		}
		printf("Client connection found. Handling request!\n");
		
		pid = fork();
		if (pid < 0) {
			perror("fork");
			continue;
		} else if (pid == 0) {
			printf("Child process(PID=%d) spawned to handle client connection!\n", getpid());
			handle_client_connection(client_sfd);
			printf("\n");
			exit(EXIT_SUCCESS);
		}
	}
}
