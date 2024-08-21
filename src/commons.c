#include "include/commons.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *trim(const char *str) {
  size_t len = strlen(str);
  size_t start = 0;

  while (isspace(str[start])) {
    start++;
  }
  while (isspace(str[len - 1])) {
    len--;
  }

  char *res = calloc((len - start), sizeof(char));
  strncpy(res, &str[start], len - start);
  return res;
}

char **split(char *str, char delim, size_t *arrLen, size_t maxSplit) {
  size_t arrCap = maxSplit;
  char **arr = malloc(arrCap * sizeof(char *));
  size_t _arrLen = 0;
  size_t strLen = strlen(str);

  size_t prevDelim = -1;
  for (size_t i = 0; _arrLen < arrCap - 1 && i < strLen; i++) {
    if (str[i] == delim) {
      arr[_arrLen] = calloc((i - (prevDelim + 1)), sizeof(char *));
      strncpy(arr[_arrLen++], &str[prevDelim + 1], i - (prevDelim + 1));
      prevDelim = i;
    }
  }

  arr[_arrLen] = calloc(((strLen) - (prevDelim + 1)), sizeof(char *));
  strncpy(arr[_arrLen++], &str[prevDelim + 1], (strLen) - (prevDelim + 1));

  *arrLen = _arrLen;
  return arr;
}

char *readline(char *str, int cap, size_t *len) {
  char *line = calloc(cap, sizeof(char));
  size_t _len = 0;

  while (_len < cap - 1) {
    if (str[_len] == '\r' && str[_len + 1] == '\n') {
      _len += 2;
      break;
    }

    line[_len] = str[_len];
    _len++;
  }

  *len = _len;
  return line;
}
