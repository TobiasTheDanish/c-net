#include "include/http.h"
#include <stdio.h>
#include <unistd.h>

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
  return Response_file(StatusOk, "text/html", "index.html");
}

Response apiHandler(Request *req) {
  printf("Request body:\n%s\n", req->body);

  return Response_text(StatusOk, "Thanks for calling\n");
}
