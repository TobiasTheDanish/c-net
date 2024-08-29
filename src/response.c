#include "include/http.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Response Response_new(const char *protocol, Status status);
void Response_writeBody(Response *res, const char *body);
char *Response_toBytes(Response *res);

static const char *responseStatusText[StatusAmount] = {
    [StatusOk] = "OK",
    [StatusCreated] = "Created",
    [StatusAccepted] = "Accepted",
    [StatusNonAuthoritativeInformation] = "Non Authoritative Information",
    [StatusNoContent] = "No Content",
    [StatusResetContent] = "Reset Content",
    [StatusPartialContent] = "Partial Content",
    [StatusMultiStatus] = "Multi Status",
    [StatusAlreadyReported] = "Already Reported",
    [StatusImUsed] = "Im Used",
    [StatusMultipleChoices] = "Multiple Choices",
    [StatusMovedPermanently] = "Moved Permanently",
    [StatusFound] = "Found",
    [StatusSeeOther] = "See Other",
    [StatusNotModified] = "Not Modified",
    [StatusTemporaryRedirect] = "Temporary Redirect",
    [StatusPermanentRedirect] = "Permanent Redirect",
    [StatusBadRequest] = "Bad Request",
    [StatusUnathorized] = "Unathorized",
    [StatusPaymentRequired] = "Payment Required",
    [StatusForbidden] = "Forbidden",
    [StatusNotFound] = "Not Found",
    [StatusMethodNotAllowed] = "Method Not Allowed",
    [StatusNotAcceptable] = "Not Acceptable",
    [StatusProxyAuthenticationRequired] = "Proxy Authentication Required",
    [StatusRequestTimeout] = "Request Timeout",
    [StatusConflict] = "Conflict",
    [StatusGone] = "Gone",
    [StatusLengthRequired] = "Length Required",
    [StatusPreconditionFailed] = "Precondition Failed",
    [StatusContentTooLarge] = "Content Too Large",
    [StatusURITooLong] = "URI Too Long",
    [StatusUnsupportedMediaType] = "Unsupported Media Type",
    [StatusRangeNotSatisfiable] = "Range Not Satisfiable",
    [StatusExpectationFailed] = "Expectation Failed",
    [StatusImATeapot] = "Im A Teapot",
    [StatusMisdirectedRequest] = "Misdirected Request",
    [StatusUnprocessableContent] = "Unprocessable Content",
    [StatusLocked] = "Locked",
    [StatusFailedDependency] = "Failed Dependency",
    [StatusTooEarly] = "Too Early",
    [StatusUpgradeRequired] = "Upgrade Required",
    [StatusPreconditionRequired] = "Precondition Required",
    [StatusTooManyRequests] = "Too Many Requests",
    [StatusRequestHeaderFieldsTooLarge] = "Request Header Fields Too Large",
    [StatusUnavailableForLegalReasons] = "Unavailable For Legal Reasons",
    [StatusInternalServerError] = "Internal Server Error",
    [StatusNotImplemented] = "Not Implemented",
    [StatusBadGateway] = "Bad Gateway",
    [StatusServiceUnavailable] = "Service Unavailable",
    [StatusGatewayTimeout] = "Gateway Timeout",
    [StatusHTTPVersionNotSupported] = "HTTP Version Not Supported",
    [StatusVariantAlsoNegotiates] = "Variant Also Negotiates",
    [StatusInsufficientStorage] = "Insufficient Storage",
    [StatusLoopDetected] = "Loop Detected",
    [StatusNotExtended] = "Not Extended",
    [StatusNetworkAuthenticationRequired] = "Network Authentication Required",
};

Response Response_new(const char *protocol, Status status) {
  return (Response){
      .protocol = protocol,
      .statusCode = status,
      .statusText = responseStatusText[status],

      .headers = NULL,
      .headerCount = 0,
  };
}

Response Response_json(Status status, const char *json) {
  Response res = Response_new("HTTP/1.1", status);
  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = "application/json",
                           });
  Response_writeBody(&res, json);

  return res;
}

Response Response_html(Status status, const char *html) {
  Response res = Response_new("HTTP/1.1", status);
  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = "text/html",
                           });
  Response_writeBody(&res, html);

  return res;
}

Response Response_text(Status status, const char *body) {
  Response res = Response_new("HTTP/1.1", status);

  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = "text/plain",
                           });
  Response_writeBody(&res, body);

  return res;
}

Response Response_file(Status status, char *contentType, const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (f == NULL) {
    return Response_text(StatusNotFound, "File not found");
  }

  char content[2048] = {0};

  if (!(fread(content, sizeof(char), 2047, f))) {
    printf("Could not read index.html\n");
    return Response_text(StatusInternalServerError, "Error reading file");
  }

  Response res = Response_new("HTTP/1.1", status);

  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = contentType,
                           });
  Response_writeBody(&res, content);

  return res;
}

void Response_writeBody(Response *res, const char *body) {
  size_t n = strlen(body);
  Header contentLength = {
      .key = "Content-Length",
      .value = calloc(20, sizeof(char)),
  };
  snprintf(contentLength.value, 20, "%zu", n);
  Response_addHeader(res, contentLength);

  res->body = malloc(n * sizeof(char));
  strncpy(res->body, body, n);

  res->bodyLength = n;
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

  resSize += res->bodyLength + 5; // 5 for final \r\n\r\n\0

  char *bytes = calloc(resSize, sizeof(char));
  int byteLen = 0;
  byteLen += snprintf(bytes, resSize, "%s %u %s\r\n", res->protocol,
                      res->statusCode, res->statusText);

  for (size_t i = 0; i < res->headerCount; i++) {
    Header h = res->headers[i];
    char headerStr[100];
    size_t headerStrLen = snprintf(headerStr, 100, "%s:%s\r\n", h.key, h.value);
    if (byteLen + headerStrLen >= resSize) {
      printf("ERROR when serializing response. Could not write header '%s', "
             "since it would overflow buffer, maybe buffer size is not "
             "calculated correctly\n",
             h.key);
      exit(1);
    }

    strncpy(&bytes[byteLen], headerStr, headerStrLen);
    byteLen += headerStrLen;
  }

  bytes[byteLen++] = '\r';
  bytes[byteLen++] = '\n';
  // TODO: fix bug with last 4 values in response overflowing after 3 requests

  strncpy(&bytes[byteLen], res->body, res->bodyLength);
  byteLen += res->bodyLength;
  strncpy(&bytes[byteLen], "\r\n\r\n", 5);
  byteLen += 4;

  return bytes;
}

void Response_addHeader(Response *res, Header h) {
  res->headers = realloc(res->headers, (res->headerCount + 1) * sizeof(Header));
  res->headers[res->headerCount++] = h;
}
