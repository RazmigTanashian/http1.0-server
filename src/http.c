#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "http.h"
#include "server.h"
#include "utils.h"

#define HEADER_SIZE 256

#define DATETIME_SIZE 64

static size_t get_datetime(char *s, int s_len) {
	time_t t = time(NULL);
        struct tm tm;
        size_t ret;

        tm = *localtime(&t);
        return strftime(s, s_len, "Date: %a, %d %b %Y %H:%M:%S GMT", &tm);
}

static char *parse_for_pathname(char *msg, int msg_len) {
	char *duplicate_msg = malloc(msg_len + 1);
	char *pathname = NULL;
	char *token = NULL;

	memcpy(duplicate_msg, msg, msg_len + 1);
	duplicate_msg[msg_len] = '\0';

	token = strtok(duplicate_msg, " ");
	if (!token) {
		perror("strtok");
		return NULL;
	}
	token = strtok(NULL, " ");
	if (!token) {
		perror("strtok");
		return NULL;
	}

	if (token[0] == '/') {
		memmove(token, &token[1], strlen(token) - 1);
		token[strlen(token) - 1] = '\0';
	}

	pathname = strndup(token, strlen(token));
	free(duplicate_msg);
	return pathname;
}

static void respond_with_internal_server_error(int client_sfd) {
	char dt[DATETIME_SIZE] = { "" };
	char r[HEADER_SIZE] = { "" };
	int total_len;

	if (get_datetime(dt, DATETIME_SIZE) < 0) {
		fprintf(stderr, "get_datetime failed!\n");
		exit(EXIT_FAILURE);
	}

	total_len = snprintf(r, HEADER_SIZE,
				"HTTP/1.0 500 Internal Server Error\r\n"
				"%s\r\n"
				"Server: Razmig's server\r\n"
				"\r\n",
				dt);
	if (total_len < 0 || total_len >= HEADER_SIZE) {
		perror("snprintf");
		exit(EXIT_FAILURE);
	}

	if (send_all(client_sfd, r, strlen(r), 0) < 0) {
		fprintf(stderr, "sendall() failed!\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

enum http_method http_retrieve_method(char *msg, int msg_len) {
	char *duplicate_msg = malloc(msg_len + 1);
	char *token = NULL;
	enum http_method m;
	memcpy(duplicate_msg, msg, msg_len + 1);

	token = strtok(duplicate_msg, " ");
	if (strcmp(token, "GET") == 0) {
		m = HTTP_METHOD_GET;
	} else if (strcmp(token, "POST") == 0) {
		m = HTTP_METHOD_POST;
	} else if (strcmp(token, "HEAD") == 0) {
		m = HTTP_METHOD_HEAD;
	} else {
		m = HTTP_METHOD_ERR_NOT_FOUND;
	}

	free(duplicate_msg);
	return m;
}

void http_handle_request_get(int client_sfd, char *msg, int msg_len) {
	// To open & read client-requested file
	char *pathname = NULL;
	FILE *fp = NULL;
	int fp_fd;
	struct stat file_stat;
	int content_len = 0;
	int total_len;
	int num_bytes_read;
	char file_buf[512];

	char dt[DATETIME_SIZE] = { "" };

	// For malloc()'ed buf
	char *response = NULL;
	int response_size = HEADER_SIZE;

	char *response_status = "HTTP/1.0 200 OK";

	int num_bytes_sent = 0;

	if ((pathname = parse_for_pathname(msg, msg_len)) == NULL) {
		respond_with_internal_server_error(client_sfd);
	}

	if ((fp = fopen(pathname, "r")) == NULL) {
		perror("fopen");
		response_status = "HTTP/1.0 204 No Content";
	} else {
		if ((fp_fd = fileno(fp)) < 0) {
			perror("fileno");
			respond_with_internal_server_error(client_sfd);
		}
		if (fstat(fp_fd, &file_stat) < 0) {
			perror("fstat");
			respond_with_internal_server_error(client_sfd);
		}
		content_len = file_stat.st_size;
	}

	response_size += content_len;
	if ((response = malloc(response_size)) == NULL) {
		perror("malloc");
		respond_with_internal_server_error(client_sfd);
	}

	if (get_datetime(dt, DATETIME_SIZE) < 0) {
		fprintf(stderr, "get_datetime failed!\n");
		respond_with_internal_server_error(client_sfd);
	}

	// If requested content was correctly fopened
	if (fp) {
		total_len = snprintf(response, HEADER_SIZE,
			"%s\r\n"
			"%s\r\n"
			"Server: Razmig's server\r\n"
			"Content-type: text/html\r\n"
			"Content-length: %zu\r\n"
			"\r\n",
			response_status,
			dt,
			(size_t)content_len);
		if (total_len < 0 || total_len >= HEADER_SIZE) {
			perror("snprintf");
			respond_with_internal_server_error(client_sfd);
		}

		// Append the body
		while ((num_bytes_read = fread(file_buf, 1, ARRAY_LENGTH(file_buf), fp)) > 0 ) {
			if (ferror(fp)) {
				perror("fread");
				respond_with_internal_server_error(client_sfd);
				break;
			}

			if (total_len + num_bytes_read > response_size) {
				break;
			}

			memcpy(response + total_len, file_buf, num_bytes_read);
			total_len += num_bytes_read;
		}

		fclose(fp);
	} else {
		total_len = snprintf(response, HEADER_SIZE,
			"%s\r\n"
			"%s\r\n"
			"Server: Razmig's server\r\n"
			"\r\n",
			response_status,
			dt);
		if (total_len < 0 || total_len >= HEADER_SIZE) {
			perror("snprintf");
			respond_with_internal_server_error(client_sfd);
		}
	}

	if (send_all(client_sfd, response, total_len, 0) == -1) {
		fprintf(stderr, "send_all() failed!\n");
	}

	free(pathname);
	free(response);

	exit(EXIT_SUCCESS);
}

void http_handle_request_post(int client_sfd, char *msg, int msg_len) {}

void http_handle_request_head(int client_sfd, char *msg, int msg_len) {}

void http_handle_request_unrecognized(int client_sfd) {
	char *r = NULL;
	char dt[DATETIME_SIZE];
	get_datetime(dt, DATETIME_SIZE);

	snprintf(r, HEADER_SIZE,
		"HTTP/1.0 501 Not Implemented\r\n"
		"%s\r\n"
		"Server: Razmig's server\r\n",
		dt);

	send_all(client_sfd, r, HEADER_SIZE, 0);
}
