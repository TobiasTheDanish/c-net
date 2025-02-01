#include "include/http.h"
#include <stdio.h>
#include <unistd.h>

Response apiHandler(Request *);
Response indexHandler(Request *req);
Response faviconHandler(Request *req);
Response routerHandler(Request *req);

int main() {
  Server *s = Server_new(8080);

  Server_addHandler(s, GET, "/", indexHandler);
  Server_addHandler(s, GET, "/favicon.ico", faviconHandler);
  Server_addHandler(s, GET, "/router.png", routerHandler);
  Server_addHandler(s, POST, "/api", apiHandler);

  Server_serveAsync(s);
  return 0;
}

Response indexHandler(Request *req) {
  return Response_textFile(StatusOk, "text/html", "index.html");
}

Response routerHandler(Request *req) {
  return Response_binaryFile(StatusOk, "image/png", "router.png");
}

Response faviconHandler(Request *req) {
  return Response_binaryFile(StatusOk, "image/png", "favicon.png");
}

Response apiHandler(Request *req) {
  printf("Request body:\n%s\n", req->body);

  return Response_text(StatusOk, "Thanks for calling\n");
}
