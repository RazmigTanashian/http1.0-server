#ifndef HTTP_H
#define HTTP_H

#define HEADER_SIZE 256

#define DATETIME_SIZE 64

enum http_method {
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,

	HTTP_METHOD_ERR_NOT_FOUND
};

struct http_request {
	enum http_method method;
	char path[64];
};

enum http_method http_retrieve_method(char *msg, int msg_len);

void http_handle_request_get(int client_sfd, char *msg, int msg_len);
void http_handle_request_post(int client_sfd, char *msg, int msg_len);
void http_handle_request_head(int client_sfd, char *msg, int msg_len);
void http_handle_request_unrecognized(int client_sfd);

#endif // HTTP_H
