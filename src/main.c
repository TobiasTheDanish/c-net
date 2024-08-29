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
  printf("IndexHandler hit\n");

  FILE *f = fopen("index.html", "r");

  if (f != NULL) {
    char content[2048] = {0};

    if (!(fread(content, sizeof(char), 2047, f))) {
      printf("Could not read index.html\n");
      return Response_text(StatusInternalServerError, "Error reading file");
    }

    return Response_html(StatusOk, content);
  } else {
    printf("Could not find index.html\n");
    return Response_text(StatusInternalServerError, "Error reading file");
  }
}

Response apiHandler(Request *req) {
  printf("Request body:\n%s\n", req->body);

  return Response_text(StatusOk, "Thanks for calling\n");
}
