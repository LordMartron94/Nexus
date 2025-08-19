/* Created by LordMartron on 20/08/2025. */

#include <stdlib.h>

char *nexus_string_duplicate(const char* sourceString) {
  size_t length = 0;
  char* duplicate = NULL;

  if (!sourceString) return NULL;

  while (sourceString[length] != '\0') {
    ++length;
  }

  duplicate = (char*)malloc(length + 1u);
  if (!duplicate) return NULL;

  {
    for (size_t i = 0; i <= length; ++i) duplicate[i] = sourceString[i];
  }
  return duplicate;
}
