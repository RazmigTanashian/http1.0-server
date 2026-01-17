#include "http.h"
#include "utils.h"
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

static size_t get_datetime(char *buf, int buf_len) {
	time_t t = time(NULL);
        struct tm tm;
        size_t ret;

        tm = *localtime(&t);
        return strftime(buf, buf_len, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &tm);
}

enum http_method http_retrieve_method(char *msg, int msg_len) {
	//struct http_request req;
	char *duplicate_msg = malloc(msg_len + 1); // Will be used by strtok()
	char *token = NULL;
	enum http_method m;
	//memset(&req, 0, sizeof(struct http_request));
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
	char *duplicate_msg = malloc(msg_len + 1);
	char *pathname = NULL;
	char *token = NULL;

	char *response_status = NULL;
	FILE *fp = NULL;
	int fp_fd;
	struct stat file_stat;
	int file_len;

	memcpy(duplicate_msg, msg, msg_len + 1);
	duplicate_msg[msg_len] = '\0';

	token = strtok(duplicate_msg, " ");
	if (!token) {
		perror("strtok");
		goto cleanup;
	}
	token = strtok(NULL, " ");
	if (!token) {
		perror("strtok");
		goto cleanup;
	}

	pathname = token;

	// Remove leading '/'
	if (pathname[0] = '/') {
		memmove(pathname, &pathname[1], strlen(pathname) - 1);
		pathname[strlen(pathname) - 1] = '\0';
	}

	fp = fopen(pathname, "r");
	if (!fp) {
		perror("fopen");
		goto cleanup;
	}
	// Create dynamic response buf
        if ( (fp_fd = fileno(fp)) < 0 ) {
                perror("fileno");
                response_status = "HTTP/1.0 500 Internal Server Error\r\n";
        }
        if (fstat(fp_fd, &file_stat) < 0) {
                perror("fstat");
                fclose(fp);
                response_status = "HTTP/1.0 500 Internal Server Error\r\n";
        }
	int r_size = 256 + file_stat.st_size;
	char *r = malloc(r_size);

	char dt[64];
	get_datetime(dt, ARRAY_LENGTH(dt));

	int header_len = snprintf(r, r_size,
		"HTTP/1.0 200 OK\r\n"
		"%s"
		"Server: Razmig's server\r\n"
		"Content-type: text/html\r\n"
		"Content-length: %zu\r\n"
		"\r\n",
		dt,
		file_stat.st_size);
	printf("%s", r);

	int total_len = header_len;
	int n;
	char file_buf[512];
	while( (n = fread(file_buf, 1, ARRAY_LENGTH(file_buf), fp)) > 0 ) {
		if (total_len + n > r_size) {
			break;
		}

		memcpy(r + total_len, file_buf, n);
		total_len += n;
	}
	fclose(fp);

	//printf("%s\n", r);

	send(client_sfd, r, total_len, 0);

cleanup:
	free(duplicate_msg);
	free(r);
}

void http_handle_request_post(int client_sfd, char *msg, int msg_len) {}

void http_handle_request_head(int client_sfd, char *msg, int msg_len) {}

