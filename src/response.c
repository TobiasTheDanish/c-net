#include "include/http.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  };
}

// Content-type
Response Response_notFound(char *body) {
  Response res = Response_new("HTTP/1.1", StatusNotFound);
  size_t bodyLen = strlen(body);
  res.bodyLength = bodyLen;

  res.headerCount = 1;
  res.headers = malloc(sizeof(Header));
  res.headers[0] = (Header){
      .key = "Content-Length",
      .value = calloc(20, sizeof(char)),
  };
  snprintf(res.headers[0].value, 20, "%lu", bodyLen);

  Response_writeBody(&res, body, bodyLen);

  return res;
}

void Response_writeBody(Response *res, char *body, size_t n) {
  res->body = malloc(n * sizeof(char));
  strncpy(res->body, body, n);
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

void Response_addHeader(Response *res, Header h) {
  res->headers = realloc(res->headers, (res->headerCount + 1) * sizeof(Header));
  res->headers[res->headerCount++] = h;
}
