/* Created by LordMartron on 20/08/2025. */

#include <nexus/nexus.h>
#include <stdarg.h>
#include <stdio.h>
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

void nexus_string_message_format_copy(char *buffer, const size_t bufferSize, const char *format, ...)
{
  if (!buffer || bufferSize == 0) return;
  va_list ap;
  va_start(ap, format);
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER < 1900
  _vsnprintf_s(buffer, bufferSize, _TRUNCATE, format, ap);  /* old MSVC */
#else
  vsnprintf(buffer, bufferSize, format, ap);                /* C99+ */
#endif
  va_end(ap);
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

void nexus_string_buffer_reset(char *buffer, const size_t bufferSize) {
  if (buffer && bufferSize) buffer[0] = NEXUS_STRING_TERMINATOR;
}