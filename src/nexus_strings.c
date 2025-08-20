/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>
#include <stdlib.h>

void nexus_string_message_copy(char* messageBuffer, const size_t messageBufferSize, const char* message) {
  size_t i = 0;
  if (!messageBuffer || messageBufferSize == 0) return;
  while (message[i] != '\0' && i + 1 < messageBufferSize) {
    messageBuffer[i] = message[i];
    ++i;
  }
  messageBuffer[i] = '\0';
}

char *nexus_string_duplicate(const char* sourceString) {
  size_t length = 0;
  char* duplicate = NULL;

  if (!sourceString) return NULL;

  while (sourceString[length] != '\0') {
    ++length;
  }

  duplicate = (char*)NEXUS_ALLOC(length + 1u);
  if (!duplicate) return NULL;

  {
    for (size_t i = 0; i <= length; ++i) duplicate[i] = sourceString[i];
  }
  return duplicate;
}
