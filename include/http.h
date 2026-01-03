#ifndef HTTP_H
#define HTTP_H

enum http_method {
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD
};

struct http_request {
	enum http_method method;
	char path[64];
	char head[64];
};

struct http_request http_parse_request(char client_msg[]);

void http_handle_request_get(void);
void http_handle_request_post(void);
void http_handle_request_head(void);

#endif // HTTP_H
