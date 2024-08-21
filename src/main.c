#include "include/http.h"

int main() {
  Server *s = Server_new(8080);
  Server_serve(s);
  return 0;
}
