#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "http.h"

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

#define PORT_NUMBER 8080

void handle_client_connection(int client_sfd) {
	char recv_buf[256] = { '\0' };
	int r = recv(client_sfd, recv_buf, ARRAY_LENGTH(recv_buf), 0);
	if (r < 0) {
		perror("recv");
		exit(EXIT_FAILURE);
	}

 	struct http_request req = http_parse_request(recv_buf);

	if (req.method == HTTP_METHOD_GET) {
		http_handle_request_get();
	} else if (req.method == HTTP_METHOD_POST) {
		http_handle_request_post();
	} else if (req.method == HTTP_METHOD_HEAD) {
		http_handle_request_head();
	} else {
		printf("Http1.0 request method received from the client was unrecognized!\n");
	}
}

void setup_server_socket(int *sock_fd) {
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

void run_server(void) {
	int server_sfd, client_sfd;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	int r; // for recv()
	char recv_buf[256] = { '\0' };

	pid_t pid;

	setup_server_socket(&server_sfd);
	if (server_sfd < 0) {
		perror("setup_server_socket");
		exit(EXIT_FAILURE);
	}

	while(1) {
		printf("Waiting for a client connection...\n");
		if ((client_sfd = accept(server_sfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
			perror("accept");
			continue;
		}
		
		pid = fork();
		if (pid < 0) {
			perror("fork");
			continue;
		} else if (pid == 0) {
			printf("Child process(PID=%d) spawned to handle client connection!\n", getpid());
			handle_client_connection(client_sfd);
			exit(EXIT_SUCCESS);
		}
	}
}
