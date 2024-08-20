#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct HTTP_HEADER {
  char *key;
  char *value;
} Header;

typedef struct HTTP_REQUEST {
  char *protocol;
  char *method;
  char *path;

  Header *headers;
  size_t headerCount;

  char *body;
} Request;

typedef struct RAW_MESSAGE {
  char *buffer;
  size_t len;
  size_t cap;
} RawMessage;

char *readline(char *str, int cap, size_t *len);
char **split(char *str, char delim, size_t *arrLen, size_t maxSplit);
char *trim(char *str);

void Request_addHeader(Request *req, Header h) {
  req->headers = realloc(req->headers, (req->headerCount + 1) * sizeof(Header));
  req->headers[req->headerCount++] = h;
}

Header *Request_getHeader(Request *req, const char *key) {
  for (size_t i = 0; i < req->headerCount; i++) {
    if (strcmp(req->headers[i].key, key) == 0) {
      return &req->headers[i];
    }
  }

  return NULL;
}

Request RawMessage_parse(RawMessage *msg) {
  size_t i = 0;
  char *httpLine = readline(msg->buffer, msg->len, &i);
  size_t wordCount = 0;
  char **words = split(httpLine, ' ', &wordCount, 3);
  free(httpLine);

  if (wordCount < 3) {
    return (Request){0};
  }

  Request req = {
      .method = words[0],
      .path = words[1],
      .protocol = words[2],
  };

  while (msg->buffer[i] != '\0' && i < msg->len) {
    size_t lineLen = 0;
    char *line = readline(&(msg->buffer[i]), (msg->len - i), &lineLen);
    if (lineLen <= 2) {
      i += lineLen;
      break;
    }
    size_t wordCount = 0;
    char **headerArr = split(line, ':', &wordCount, 2);
    free(line);

    Header h = {
        .key = headerArr[0],
        .value = trim(headerArr[1]),
    };
    Request_addHeader(&req, h);

    i += lineLen;
  }

  Header *contentLength = Request_getHeader(&req, "Content-Length");
  if (contentLength == NULL) {
    req.body = "";
  } else {
    int bodySize = atoi(contentLength->value);
    req.body = calloc(bodySize + 1, sizeof(char));
    strncpy(req.body, &msg->buffer[i], bodySize);
  }

  return req;
}

char *trim(char *str) {
  size_t len = strlen(str);
  size_t start = 0;

  while (isspace(str[start])) {
    start++;
  }
  while (isspace(str[len - 1])) {
    len--;
  }

  char *res = malloc((len - start) * sizeof(char));
  strncpy(res, &str[start], len - start);
  return res;
}

char **split(char *str, char delim, size_t *arrLen, size_t maxSplit) {
  size_t arrCap = maxSplit;
  char **arr = malloc(arrCap * sizeof(char *));
  size_t _arrLen = 0;
  size_t strLen = strlen(str);

  size_t prevDelim = -1;
  for (size_t i = 0; _arrLen < arrCap - 1 && i < strLen; i++) {
    if (str[i] == delim) {
      arr[_arrLen] = malloc((i - (prevDelim + 1)) * sizeof(char *));
      strncpy(arr[_arrLen++], &str[prevDelim + 1], i - (prevDelim + 1));
      prevDelim = i;
    }
  }

  arr[_arrLen] = malloc(((strLen) - (prevDelim + 1)) * sizeof(char *));
  strncpy(arr[_arrLen++], &str[prevDelim + 1], (strLen) - (prevDelim + 1));

  *arrLen = _arrLen;
  return arr;
}

char *readline(char *str, int cap, size_t *len) {
  char *line = calloc(cap, sizeof(char));
  size_t _len = 0;

  while (_len < cap - 1) {
    if (str[_len] == '\r' && str[_len + 1] == '\n') {
      _len += 2;
      break;
    }

    line[_len] = str[_len];
    _len++;
  }

  *len = _len;
  return line;
}

unsigned char isRunning = 1;

void intHandler(int sig) { isRunning = 0; }

int main() {
  signal(SIGINT, intHandler);

  int opt = 1;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    printf("ERROR: Could not create socket\n");
    return 1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    printf("ERROR: Could not set socket options\n");
    return 1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    printf("ERROR: Could not bind to socket\n");
    return 1;
  }

  if (listen(sockfd, 3) < 0) {
    printf("ERROR: Could not listen to socket\n");
    return 1;
  }
  socklen_t addrlen = sizeof(address);

  while (isRunning) {
    int new_socket = accept(sockfd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0) {
      printf("ERROR: Could not accept connection to socket\n");
      return 1;
    }

    int cap = 1024;
    char *buffer = calloc(cap, sizeof(char));
    int valread = read(new_socket, buffer, cap - 1);

    printf("valread: %d\n%s\n", valread, buffer);
    while (valread == 1023) {
      cap += 1024;
      buffer = realloc(buffer, cap * sizeof(char));
      valread = read(new_socket, &buffer[cap - 1024], 1023);

      printf("valread: %d\n%s\n", valread, buffer);
    }
    RawMessage rawMsg = {
        .buffer = buffer,
        .cap = cap,
        .len = valread,
    };

    Request req = RawMessage_parse(&rawMsg);
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

    int valWriten = write(new_socket, res, strlen(res));

    printf("valWriten: %d\n", valWriten);

    close(new_socket);
  }

  close(sockfd);

  return 0;
}
