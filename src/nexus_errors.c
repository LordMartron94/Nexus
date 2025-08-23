/* Created by LordMartron on 23/08/2025. */

#include <nexus/nexus.h>

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

