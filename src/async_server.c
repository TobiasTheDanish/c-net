#include "include/http.h"
#include <aio.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

extern char *Response_toBytes(Response *res, size_t *size);

typedef enum ASYNC_IO_STATE {
  INACTIVE = 0,
  READ_CONN,
  WRITE_CONN,
} AsyncIOState;

typedef struct ASYNC_IO_ENTRY {
  AsyncIOState state;
  int conn;
  void *buf;
  size_t offset;
  size_t nbytes;
} AsyncIOEntry;

#define MAX_ASYNC_ENTRIES (32)
AsyncIOEntry entries[MAX_ASYNC_ENTRIES] = {0};

size_t getNextEntrySpot() {
  for (size_t i = 0; i < MAX_ASYNC_ENTRIES; i++) {
    if (entries[i].state == INACTIVE) {
      return i;
    }
  }
  printf("The max number of active async operations have been reached. Make "
         "sure to deativate after a "
         "request have been processed\nShutting down\n");
  exit(1);
}

typedef struct REQUEST_ENTRY {
  unsigned char active;
  int conn;
  Request req;
} RequestEntry;
RequestEntry requestQueue[MAX_ASYNC_ENTRIES] = {0};
void addRequestToQueue(int conn, Request req) {
  for (size_t i = 0; i < MAX_ASYNC_ENTRIES; i++) {
    if (!requestQueue[i].active) {
      memcpy(&requestQueue[i].req, &req, sizeof(Request));
      requestQueue[i].conn = conn;
      requestQueue[i].active = 1;
      return;
    }
  }
  printf("Too many active async operations, make sure to deativate after a "
         "request have been processed\nShutting down\n");
  exit(1);
}

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
void Server_readConnectionAsync(AsyncIOEntry *entry);

void Server_serveAsync(Server *s) {
  if (listen(s->socket, 3) < 0) {
    printf("ERROR: Could not listen to socket\n");
    exit(1);
  }
  socklen_t addrlen = sizeof(s->address);

  printf("Server listening on port '%d'\n", s->port);

  struct pollfd pollDescriptor = {
      .fd = s->socket,
      .events = POLLIN,
  };

  while (1) {
    {
      int pollRes = poll(&pollDescriptor, 1, 10);
      if (pollRes < 0) {
        printf("ERROR: Could not poll socket for incomming connections\n");
        return;
      }

      if (pollRes > 0 && pollDescriptor.revents & POLLIN) {
        printf("pollRes: %d\n", pollRes);
        for (; pollRes > 0; pollRes--) {
          int new_socket =
              accept(s->socket, (struct sockaddr *)&s->address, &addrlen);
          if (new_socket < 0) {
            printf("ERROR: Could not accept connection to socket\n");
            exit(1);
          }

          size_t index = getNextEntrySpot();

          printf("New connection for spot %zu\n", index);

          entries[index].state = READ_CONN;
          entries[index].conn = new_socket;
          entries[index].buf = calloc(1024, sizeof(char));
          entries[index].nbytes = 1023;
          entries[index].offset = 0;
        }
      }
    }

    {
      struct pollfd p = {0};
      // check active async io
      for (size_t i = 0; i < MAX_ASYNC_ENTRIES; i++) {
        switch (entries[i].state) {
        case INACTIVE:
          continue;
        case READ_CONN: {
          p.fd = entries[i].conn;
          p.events = POLLIN;

          int pollRes = poll(&p, 1, 10);
          if (pollRes < 0) {
            printf("ERROR: Could not poll socket for incomming connections\n");
            return;
          }

          if (pollRes > 0 && p.revents & POLLIN) {
            int valRead =
                read(entries[i].conn, (entries[i].buf + entries[i].offset),
                     entries[i].nbytes);

            entries[i].offset += valRead;
            if (valRead == entries[i].nbytes) {
              printf("data read on entry #%zu hit max, will read again\n", i);
              size_t cap = entries[i].offset + entries[i].nbytes;
              entries[i].buf = realloc(entries[i].buf, cap * sizeof(char));
            } else {
              addRequestToQueue(
                  entries[i].conn,
                  Request_parse(entries[i].buf, entries[i].offset));

              entries[i].state = INACTIVE;
              free(entries[i].buf);
              entries[i].buf = NULL;
            }
          }
        } break;
        case WRITE_CONN: {
          p.fd = entries[i].conn;
          p.events = POLLOUT;

          int pollRes = poll(&p, 1, 10);
          if (pollRes < 0) {
            printf("ERROR: Could not poll socket for incomming connections\n");
            return;
          }
          if (pollRes > 0 && p.revents & POLLOUT) {
            write(entries[i].conn, entries[i].buf + entries[i].offset,
                  entries[i].nbytes);
            entries[i].state = INACTIVE;
            free(entries[i].buf);
            entries[i].buf = NULL;

            close(entries[i].conn);
          }
        } break;
        }
      }
    }

    for (size_t i = 0; i < MAX_ASYNC_ENTRIES; i++) {
      if (requestQueue[i].active) {
        Response response;

        if (requestQueue[i].req.method == UNSUPPORTED) {
          response =
              Response_text(StatusMethodNotAllowed, "Method not supported");
        } else {
          RequestHandler h = Server_getHandler(s, requestQueue[i].req.method,
                                               requestQueue[i].req.path);
          if (h == NULL) {
            response = Response_text(StatusNotFound, "Resource not found");
          } else {
            response = h(&requestQueue[i].req);
          }
        }

        printf("%s %s: %d \n", Request_methodName(requestQueue[i].req.method),
               requestQueue[i].req.path, response.statusCode);

        free(requestQueue[i].req.body);

        size_t resSize = 0;
        char *res = Response_toBytes(&response, &resSize);

        free(response.headers);
        free(response.body);

        size_t spot = getNextEntrySpot();

        entries[spot].state = WRITE_CONN;
        entries[spot].conn = requestQueue[i].conn;
        entries[spot].buf = res;
        entries[spot].nbytes = resSize;
        entries[spot].offset = 0;

        requestQueue[i].active = 0;
      }
    }
  }
}

/*
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
*/
