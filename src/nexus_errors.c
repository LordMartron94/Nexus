/* Created by LordMartron on 23/08/2025. */

#include <nexus/nexus.h>
#include <string.h>

void nexus_errors_ok(char* errorBuffer, const nexus_u64 errorBufferSize) {
  if (errorBuffer && errorBufferSize) errorBuffer[0] = NEXUS_STRING_TERMINATOR;
}

NEXUS_ERROR_CODE nexus_errors_invalid_argument(char* errorBuffer, const nexus_u64 errorBufferSize) {
  nexus_string_message_copy(errorBuffer, errorBufferSize, "Invalid argument");
  return NEXUS_INVALID_ARGUMENT;
}

NEXUS_ERROR_CODE nexus_errors_out_of_memory(char* errorBuffer, const nexus_u64 errorBufferSize) {
  nexus_string_message_copy(errorBuffer, errorBufferSize, "Out of memory");
  return NEXUS_FUNCTION_ERROR;
}

void nexus_errors_error_number_format(char *errorBuffer,
                                const nexus_u64 errorBufferSize,
                                const char *fallback,
                                const int errorNumber)
{
  if (!errorBuffer || errorBufferSize == 0) return;
#if defined(_MSC_VER)
  if (strerror_s(errorBuffer, errorBufferSize, errorNumber) != 0) {
    nexus_string_message_copy(errorBuffer, errorBufferSize, fallback);
  }
#else
  const char *msg = strerror(errnum);
  if (msg && *msg) {
    nexus_string_message_copy(errorBuffer, errorBufferSize, msg);
  } else {
    nexus_string_message_copy(errorBuffer, errorBufferSize, fallback);
  }
#endif
}

