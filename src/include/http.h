#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>

typedef enum HTTP_METHOD {
  UNSUPPORTED = 0,
  GET,
  POST,
  PUT,
  DELETE,
} Method;

typedef struct HTTP_HEADER {
  char *key;
  char *value;
} Header;

typedef struct HTTP_RESPONSE {
  const char *protocol;
  unsigned int statusCode;
  const char *statusText;

  Header *headers;
  size_t headerCount;

  char *body;
  size_t bodyLength;
} Response;
Response Response_notFound(char *body);
char *Response_toBytes(Response *res);

typedef struct HTTP_REQUEST {
  char *protocol;
  Method method;
  char *path;

  Header *headers;
  size_t headerCount;

  char *body;
  size_t bodyLength;
} Request;
void Request_addHeader(Request *req, Header h);
Header *Request_getHeader(Request *req, const char *key);
Request Request_parse(char *buffer, size_t len, size_t cap);

typedef Response (*RequestHandler)(Request *);

typedef struct HTTP_SERVER Server;

Server *Server_new(unsigned short port);
void Server_serve(Server *s);
// Example of how request handlers could look
void Server_addHandler(Server *s, Method method, const char *path,
                       RequestHandler handler);
Response IndexHandler(Request *req);
#endif // !HTTP_H
