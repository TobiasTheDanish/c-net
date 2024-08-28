#include "include/http.h"
#include <stdio.h>

Response apiHandler(Request *);

int main() {
  Server *s = Server_new(8080);

  Server_addHandler(s, POST, "/api", apiHandler);

  Server_serve(s);
  return 0;
}

Response apiHandler(Request *req) {
  printf("Request body:\n %s\n", req->body);

  return Response_text(StatusOk, "Thanks for calling\n");
}
