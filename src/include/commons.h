#ifndef COMMONS_H
#define COMMONS_H
#include <stddef.h>

char *readline(char *str, int cap, size_t *len);
char **split(char *str, char delim, size_t *arrLen, size_t maxSplit);
char *trim(const char *str);

#endif // !COMMONS_H
