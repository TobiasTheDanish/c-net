#include "include/http.h"
#include <stdio.h>

Response apiHandler(Request *);
Response indexHandler(Request *req);

int main() {
  Server *s = Server_new(8080);

  Server_addHandler(s, GET, "/", indexHandler);
  Server_addHandler(s, POST, "/api", apiHandler);

  Server_serve(s);
  return 0;
}

Response indexHandler(Request *req) {
  return Response_text(StatusOk, "This is the homepage");
}

Response apiHandler(Request *req) {
  printf("Request body:\n%s\n", req->body);

  return Response_text(StatusOk, "Thanks for calling\n");
}
