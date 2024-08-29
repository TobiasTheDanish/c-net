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

typedef enum HTTP_STATUS {
  StatusOk = 200,
  StatusCreated,
  StatusAccepted,
  StatusNonAuthoritativeInformation,
  StatusNoContent,
  StatusResetContent,
  StatusPartialContent,
  StatusMultiStatus,
  StatusAlreadyReported,
  StatusImUsed = 226,
  StatusMultipleChoices = 300,
  StatusMovedPermanently,
  StatusFound,
  StatusSeeOther,
  StatusNotModified,
  StatusTemporaryRedirect = 307,
  StatusPermanentRedirect,
  StatusBadRequest = 400,
  StatusUnathorized,
  StatusPaymentRequired,
  StatusForbidden,
  StatusNotFound,
  StatusMethodNotAllowed,
  StatusNotAcceptable,
  StatusProxyAuthenticationRequired,
  StatusRequestTimeout,
  StatusConflict,
  StatusGone,
  StatusLengthRequired,
  StatusPreconditionFailed,
  StatusContentTooLarge,
  StatusURITooLong,
  StatusUnsupportedMediaType,
  StatusRangeNotSatisfiable,
  StatusExpectationFailed,
  StatusImATeapot,
  StatusMisdirectedRequest = 421,
  StatusUnprocessableContent,
  StatusLocked,
  StatusFailedDependency,
  StatusTooEarly,
  StatusUpgradeRequired,
  StatusPreconditionRequired = 428,
  StatusTooManyRequests,
  StatusRequestHeaderFieldsTooLarge = 431,
  StatusUnavailableForLegalReasons,
  StatusInternalServerError = 500,
  StatusNotImplemented,
  StatusBadGateway,
  StatusServiceUnavailable,
  StatusGatewayTimeout,
  StatusHTTPVersionNotSupported,
  StatusVariantAlsoNegotiates,
  StatusInsufficientStorage,
  StatusLoopDetected,
  StatusNotExtended = 510,
  StatusNetworkAuthenticationRequired,
  StatusAmount,
} Status;

typedef struct HTTP_RESPONSE {
  const char *protocol;
  unsigned int statusCode;
  const char *statusText;

  Header *headers;
  size_t headerCount;

  char *body;
  size_t bodyLength;
} Response;
Response Response_text(Status status, const char *body);
Response Response_html(Status status, const char *body);
Response Response_json(Status status, const char *json);
Response Response_file(Status status, char *contentType, const char *filepath);
void Response_addHeader(Response *res, Header h);

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
