#include "include/commons.h"
#include "include/http.h"
#include <stdlib.h>
#include <string.h>

const char *Request_methodName(Method m) {
  switch (m) {
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  default:
    return "UNSUPPORTED";
  }
}

Request Request_parse(char *buffer, size_t len, size_t cap) {
  size_t i = 0;
  char *requestLine = readline(buffer, len, &i);
  size_t wordCount = 0;
  char **words = split(requestLine, ' ', &wordCount, 3);
  free(requestLine);

  if (wordCount < 3) {
    return (Request){0};
  }
  Method method;
  if (strcmp(words[0], "GET") == 0) {
    method = GET;
  } else if (strcmp(words[0], "POST") == 0) {
    method = POST;
  } else if (strcmp(words[0], "PUT") == 0) {
    method = PUT;
  } else if (strcmp(words[0], "DELETE") == 0) {
    method = DELETE;
  }

  Request req = {
      .method = method,
      .path = words[1],
      .protocol = words[2],

      .headers = NULL,
      .headerCount = 0,

      .body = NULL,
      .bodyLength = 0,
  };

  while (buffer[i] != '\0' && i < len) {
    size_t lineLen = 0;
    char *line = readline(&(buffer[i]), (len - i), &lineLen);
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
    free(headerArr[1]);
    Request_addHeader(&req, h);

    i += lineLen;
  }

  Header *contentLength = Request_getHeader(&req, "Content-Length");
  if (contentLength != NULL) {
    req.bodyLength = atoi(contentLength->value);
    req.body = calloc(req.bodyLength + 1, sizeof(char));
    strncpy(req.body, &buffer[i], req.bodyLength);
  }

  return req;
}

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
