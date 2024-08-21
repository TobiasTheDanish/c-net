#include "include/http.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct HTTP_SERVER {
  int socket;
  unsigned short port;
  struct sockaddr_in address;
} Server;

void Server_handleConnection(Server *s, int conn);

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
  return s;
}

void Server_serve(Server *s) {
  if (listen(s->socket, 3) < 0) {
    printf("ERROR: Could not listen to socket\n");
    exit(1);
  }
  socklen_t addrlen = sizeof(s->address);

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

  Request req = Request_parse(buffer, cap, len);
  printf("req: (method: %s, path: %s, protocol: %s)\n", req.method, req.path,
         req.protocol);

  for (size_t j = 0; j < req.headerCount; j++) {
    Header h = req.headers[j];
    printf("Header #%zu: ('%s': '%s')\n", j, h.key, h.value);
    free(h.key);
    free(h.value);
  }

  printf("Body: %s\n", req.body);

  const char *res = "HTTP/1.1 204 No Content\r\n\r\n";

  int valWriten = write(conn, res, strlen(res));

  printf("valWriten: %d\n", valWriten);

  close(conn);
}
