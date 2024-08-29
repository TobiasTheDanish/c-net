#include "include/http.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

extern char *Response_toBytes(Response *res);

typedef struct HTTP_HANDLER_NODE {
  Method method;
  const char *path;
  RequestHandler handler;
} HandlerNode;

typedef struct HTTP_SERVER {
  int socket;
  unsigned short port;
  struct sockaddr_in address;
  HandlerNode *handlers;
  size_t handlerCount;
} Server;

RequestHandler Server_getHandler(Server *s, Method method, const char *path);
void Server_handleConnection(Server *s, int conn);
char *Server_readConnection(int conn, size_t *len, size_t *cap);

Server *Server_new(unsigned short port) {
  int opt = 1;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("ERROR: Could not create socket\n");
    exit(1);
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    printf("ERROR: Could not set socket options\n");
    exit(1);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    printf("ERROR: Could not bind to socket\n");
    exit(1);
  }

  Server *s = malloc(sizeof(Server));
  s->socket = sockfd;
  s->address = address;
  s->port = port;
  s->handlers = NULL;
  return s;
}

void Server_serve(Server *s) {
  if (listen(s->socket, 3) < 0) {
    printf("ERROR: Could not listen to socket\n");
    exit(1);
  }
  socklen_t addrlen = sizeof(s->address);

  printf("Server listening on port '%d'\n", s->port);

  while (1) {
    int new_socket =
        accept(s->socket, (struct sockaddr *)&s->address, &addrlen);
    if (new_socket < 0) {
      printf("ERROR: Could not accept connection to socket\n");
      exit(1);
    }

    Server_handleConnection(s, new_socket);
  }
}

void Server_handleConnection(Server *s, int conn) {
  size_t len, cap;
  char *buffer = Server_readConnection(conn, &len, &cap);

  Request req = Request_parse(buffer, cap, len);
  if (req.method == UNSUPPORTED) {
    // TODO: Handle this better at some point
    close(conn);
    return;
  }

  Response response;
  RequestHandler h = Server_getHandler(s, req.method, req.path);
  if (h == NULL) {
    response = Response_text(StatusNotFound, "Resource not found");
  } else {
    response = h(&req);
  }

  char *res = Response_toBytes(&response);

  write(conn, res, strlen(res));
  free(req.headers);
  free(req.body);

  free(res);
  free(buffer);

  close(conn);
}

char *Server_readConnection(int conn, size_t *_len, size_t *_cap) {
  int cap = 1024;
  char *buffer = calloc(cap, sizeof(char));
  int valread = read(conn, buffer, cap - 1);
  int len = valread;

  while (valread == 1023) {
    cap += 1024;
    buffer = realloc(buffer, cap * sizeof(char));
    valread = read(conn, buffer + len, 1023);
    len += valread;
  }

  *_len = len;
  *_cap = cap;
  return buffer;
}

void Server_addHandler(Server *s, Method method, const char *path,
                       RequestHandler handler) {
  s->handlers =
      realloc(s->handlers, (s->handlerCount + 1) * sizeof(HandlerNode));
  s->handlers[s->handlerCount++] = (HandlerNode){
      .method = method,
      .path = path,
      .handler = handler,
  };
}

RequestHandler Server_getHandler(Server *s, Method m, const char *path) {
  for (size_t i = 0; i < s->handlerCount; i++) {
    HandlerNode n = s->handlers[i];
    if (n.method == m && strcmp(n.path, path) == 0) {
      return n.handler;
    }
  }

  return NULL;
}
