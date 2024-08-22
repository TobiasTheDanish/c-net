#include "include/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
Response Response_notFound(char *body) {
  size_t bodyLen = strlen(body);

  Response res = {
      .protocol = "HTTP/1.1",
      .statusCode = 404,
      .statusText = "Not Found",

      .headers = malloc(sizeof(Header)),
      .headerCount = 1,

      .bodyLength = bodyLen,
  };

  res.headers[0] = (Header){
      .key = "Content-Length",
      .value = calloc(20, sizeof(char)),
  };
  snprintf(res.headers[0].value, 20, "%lu", bodyLen);

  res.body = malloc(bodyLen * sizeof(char));
  strncpy(res.body, body, bodyLen);

  return res;
}

char *Response_toBytes(Response *res) {
  int resSize = strlen(res->protocol);
  resSize += strlen(res->statusText);
  resSize += 3; // the statuscode
  resSize += 4; // spaces and \r\n

  for (size_t i = 0; i < res->headerCount; i++) {
    resSize += strlen(res->headers[i].key) + strlen(res->headers[i].value) +
               3; // 1 for the ':' and 2 for \r\n
  }
  resSize += 2; // for extra \r\n

  resSize += res->bodyLength + 4; // 4 for final \r\n\r\n

  char *bytes = calloc(resSize + 1, sizeof(char));
  int byteLen = 0;
  byteLen += snprintf(bytes, resSize, "%s %u %s\r\n", res->protocol,
                      res->statusCode, res->statusText);

  for (size_t i = 0; i < res->headerCount; i++) {
    Header h = res->headers[i];
    byteLen += snprintf(&bytes[byteLen], resSize - byteLen, "%s:%s\r\n", h.key,
                        h.value);
  }

  byteLen +=
      snprintf(&bytes[byteLen], resSize - byteLen, "\r\n%s\r\n\r\n", res->body);

  return bytes;
}
