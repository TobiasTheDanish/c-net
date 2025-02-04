#include "include/http.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Response Response_new(const char *protocol, Status status);
void Response_writeBody(Response *res, const char *body, size_t n);
char *Response_toBytes(Response *res, size_t *size);

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
  Response_writeBody(&res, json, strlen(json));

  return res;
}

Response Response_html(Status status, const char *html) {
  Response res = Response_new("HTTP/1.1", status);
  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = "text/html",
                           });
  Response_writeBody(&res, html, strlen(html));

  return res;
}

Response Response_text(Status status, const char *body) {
  Response res = Response_new("HTTP/1.1", status);

  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = "text/plain",
                           });
  Response_writeBody(&res, body, strlen(body));

  return res;
}

Response Response_binaryFile(Status status, char *contentType,
                             const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (f == NULL) {
    return Response_text(StatusNotFound, "File not found");
  }

  char *content = NULL;
  size_t contentLength = 0;

  while (!feof(f)) {
    content = realloc(content, (contentLength + 2048) * sizeof(char));
    contentLength += fread(&content[contentLength], sizeof(char), 2048, f);
  }

  if (contentLength == 0) {
    printf("Could not read %s\n", filepath);
    return Response_text(StatusInternalServerError, "Error reading file");
  }

  fclose(f);

  Response res = Response_new("HTTP/1.1", status);

  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = contentType,
                           });
  Response_writeBody(&res, content, contentLength);
  free(content);

  return res;
}

Response Response_textFile(Status status, char *contentType,
                           const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (f == NULL) {
    return Response_text(StatusNotFound, "File not found");
  }

  char *content = NULL;
  size_t contentLength = 0;

  while (!feof(f)) {
    content = realloc(content, (contentLength + 2048) * sizeof(char));
    contentLength += fread(&content[contentLength], sizeof(char), 2047, f);
  }

  if (contentLength == 0) {
    printf("Could not read %s\n", filepath);
    return Response_text(StatusInternalServerError, "Error reading file");
  }

  fclose(f);

  Response res = Response_new("HTTP/1.1", status);

  Response_addHeader(&res, (Header){
                               .key = "Content-Type",
                               .value = contentType,
                           });
  Response_writeBody(&res, content, contentLength);
  free(content);

  return res;
}

void Response_writeBody(Response *res, const char *body, size_t n) {
  if (n == 0)
    n = strlen(body);
  Header contentLength = {
      .key = "Content-Length",
      .value = calloc(20, sizeof(char)),
  };
  snprintf(contentLength.value, 20, "%zu", n);
  Response_addHeader(res, contentLength);

  res->body = calloc(n, sizeof(char));
  memcpy(res->body, body, n);

  res->bodyLength = n;
}

char *Response_toBytes(Response *res, size_t *size) {
  size_t resSize =
      strlen(res->protocol) + 10; // protocol + status code + status text
  resSize += strlen(res->statusText) + 4; // spaces and \r\n

  for (size_t i = 0; i < res->headerCount; i++) {
    resSize += strlen(res->headers[i].key) + strlen(res->headers[i].value) + 4;
  }

  resSize += 4; // Extra \r\n after headers
  resSize += res->bodyLength;
  resSize += 4; // Final \r\n\r\n after the body

  char *bytes = malloc(resSize);
  if (bytes == NULL) {
    perror("Failed to allocate memory");
    exit(EXIT_FAILURE);
  }

  int byteLen = snprintf(bytes, resSize, "%s %u %s\r\n", res->protocol,
                         res->statusCode, res->statusText);

  for (size_t i = 0; i < res->headerCount; i++) {
    Header h = res->headers[i];
    byteLen += snprintf(bytes + byteLen, resSize - byteLen, "%s: %s\r\n", h.key,
                        h.value);
  }

  // Add the extra \r\n to separate headers and body
  bytes[byteLen++] = '\r';
  bytes[byteLen++] = '\n';

  memcpy(bytes + byteLen, res->body, res->bodyLength);
  byteLen += res->bodyLength;

  // Add the final \r\n\r\n at the end of the body
  bytes[byteLen++] = '\r';
  bytes[byteLen++] = '\n';
  bytes[byteLen++] = '\r';
  bytes[byteLen++] = '\n';

  *size = byteLen;

  return bytes;
}

void Response_addHeader(Response *res, Header h) {
  res->headers = realloc(res->headers, (res->headerCount + 1) * sizeof(Header));
  res->headers[res->headerCount++] = h;
}
